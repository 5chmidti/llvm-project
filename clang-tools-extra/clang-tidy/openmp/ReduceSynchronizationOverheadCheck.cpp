//===--- ReduceSynchronizationOverheadCheck.cpp - clang-tidy --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ReduceSynchronizationOverheadCheck.h"
#include "../utils/OpenMP.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/OpenMPKinds.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/Frontend/OpenMP/OMPConstants.h"
#include <cstdint>
#include <vector>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<
    Stmt, CompoundAssignOperator>
    compoundAssignOperator;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPCriticalDirective>
    ompCriticalDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPAtomicDirective>
    ompAtomicDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPSharedClause>
    ompSharedClause;
// NOLINTEND(readability-identifier-naming)

AST_MATCHER(BinaryOperator, isAllowedBinaryOperator) {
  switch (Node.getOpcode()) {
  case clang::BinaryOperatorKind::BO_Add:
  case clang::BinaryOperatorKind::BO_Mul:
  case clang::BinaryOperatorKind::BO_Sub:
  case clang::BinaryOperatorKind::BO_Div:
  case clang::BinaryOperatorKind::BO_And:
  case clang::BinaryOperatorKind::BO_Xor:
  case clang::BinaryOperatorKind::BO_Or:
  case clang::BinaryOperatorKind::BO_Shl:
  case clang::BinaryOperatorKind::BO_Shr:
    return true;
  default:
    return false;
  }
}

AST_MATCHER(BinaryOperator, isAllowedCompoundOperator) {
  switch (Node.getOpcode()) {
  case clang::BinaryOperatorKind::BO_AddAssign:
  case clang::BinaryOperatorKind::BO_MulAssign:
  case clang::BinaryOperatorKind::BO_SubAssign:
  case clang::BinaryOperatorKind::BO_DivAssign:
  case clang::BinaryOperatorKind::BO_AndAssign:
  case clang::BinaryOperatorKind::BO_XorAssign:
  case clang::BinaryOperatorKind::BO_OrAssign:
  case clang::BinaryOperatorKind::BO_ShlAssign:
  case clang::BinaryOperatorKind::BO_ShrAssign:
    return true;
  default:
    return false;
  }
}

AST_MATCHER(CompoundStmt, hasSingleStatement) { return Node.size() == 1U; }

AST_MATCHER_P2(CompoundStmt, hasStatements,
               ast_matchers::internal::Matcher<Stmt>, Statement1,
               ast_matchers::internal::Matcher<Stmt>, Statement2) {
  if (Node.size() != 2)
    return false;

  const Stmt *const Stmt1 = Node.body_front();
  const Stmt *const Stmt2 = Node.body_back();
  return Statement1.matches(*Stmt1, Finder, Builder) &&
         Statement2.matches(*Stmt2, Finder, Builder);
}

AST_MATCHER(OMPExecutableDirective, isOpenMPParallelDirective) {
  return clang::isOpenMPParallelDirective(Node.getDirectiveKind());
}

AST_MATCHER_P(OMPExecutableDirective, hasSharedVariable,
              ast_matchers::internal::Matcher<Expr>, InnerMatcher) {
  const auto *const Shared = Node.getSingleClause<OMPSharedClause>();
  if (Shared)
    for (const Expr *const VarExpr : Shared->getVarRefs()) {
      auto Result = *Builder;
      if (InnerMatcher.matches(*VarExpr, Finder, &Result)) {
        *Builder = std::move(Result);
        return true;
      }
    }

  const auto *const Private = Node.getSingleClause<OMPPrivateClause>();
  if (Private)
    for (const Expr *const VarExpr : Private->getVarRefs()) {
      auto Result = *Builder;
      if (InnerMatcher.matches(*VarExpr, Finder, &Result)) {
        *Builder = std::move(Result);
        return false;
      }
    }

  const auto *const Firstprivate =
      Node.getSingleClause<OMPFirstprivateClause>();
  if (Firstprivate)
    for (const Expr *const VarExpr : Firstprivate->getVarRefs()) {
      auto Result = *Builder;
      if (InnerMatcher.matches(*VarExpr, Finder, &Result)) {
        *Builder = std::move(Result);
        return false;
      }
    }

  const auto *const Lastprivate = Node.getSingleClause<OMPLastprivateClause>();
  if (Lastprivate)
    for (const Expr *const VarExpr : Lastprivate->getVarRefs()) {
      auto Result = *Builder;
      if (InnerMatcher.matches(*VarExpr, Finder, &Result)) {
        *Builder = std::move(Result);
        return false;
      }
    }

  const auto *const Default = Node.getSingleClause<OMPDefaultClause>();
  if (!Default)
    return true;
  if (Default->getDefaultKind() == llvm::omp::DefaultKind::OMP_DEFAULT_shared)
    return true;

  return false;
}

enum class AtomicKind : std::uint8_t {
  Unknown,
  Read,
  Write,
  Update,
  Capture,
};

struct DiagnosticData {
  AtomicKind Kind = AtomicKind::Unknown;
  SourceRange Range;
  std::vector<FixItHint> Fixes;
};

DiagnosticData getAtomicDirective(const BoundNodes &Result) {
  const auto *Critical = Result.getNodeAs<OMPCriticalDirective>("critical");
  const auto *ExtraCompound = Result.getNodeAs<CompoundStmt>("extra-compound");

  if (const auto *Read = Result.getNodeAs<Stmt>("read")) {
    DiagnosticData Data;
    Data.Kind = AtomicKind::Read;
    Data.Range = Read->getSourceRange();
    Data.Fixes = {FixItHint::CreateReplacement(Critical->getSourceRange(),
                                               "#pragma omp atomic read")};
    if (ExtraCompound) {
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getLBracLoc())));
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getRBracLoc())));
    }
    return Data;
  }
  if (const auto *Write = Result.getNodeAs<Stmt>("write")) {
    DiagnosticData Data;
    Data.Kind = AtomicKind::Write;
    Data.Range = Write->getSourceRange();
    Data.Fixes = {FixItHint::CreateReplacement(Critical->getSourceRange(),
                                               "#pragma omp atomic write")};
    if (ExtraCompound) {
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getLBracLoc())));
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getRBracLoc())));
    }
    return Data;
  }
  if (const auto *Update = Result.getNodeAs<Stmt>("update")) {
    DiagnosticData Data;
    Data.Kind = AtomicKind::Update;
    Data.Range = Update->getSourceRange();
    Data.Fixes = {FixItHint::CreateReplacement(Critical->getSourceRange(),
                                               "#pragma omp atomic update")};
    if (ExtraCompound) {
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getLBracLoc())));
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getRBracLoc())));
    }
    return Data;
  }
  if (const auto *Capture = Result.getNodeAs<Stmt>("capture")) {
    DiagnosticData Data;
    Data.Kind = AtomicKind::Capture;
    Data.Range = Capture->getSourceRange();
    Data.Fixes = {FixItHint::CreateReplacement(Critical->getSourceRange(),
                                               "#pragma omp atomic capture")};
    if (ExtraCompound) {
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getLBracLoc())));
      Data.Fixes.push_back(
          FixItHint::CreateRemoval(SourceRange(ExtraCompound->getRBracLoc())));
    }
    return Data;
  }

  return {};
}

auto referencesX() { return declRefExpr(declRefExpr(to(varDecl().bind("x")))); }
auto referencesV() {
  return declRefExpr(unless(equalsBoundNode("x"))).bind("v");
}
auto expression() { return expr(unless(equalsBoundNode("v"))); }

// v = x;
auto atomicRead() {
  return binaryOperator(isAssignmentOperator(), hasRHS(referencesX()),
                        hasLHS(referencesV()));
}

// x = expr;
auto atomicWrite() {
  return binaryOperator(isAssignmentOperator(), hasLHS(referencesX()),
                        hasRHS(expression()));
}

// x++;
// x--;
// ++x;
// --x;
// x binop= expr;
// x = x binop expr;
// x = expr binop x;
auto atomicUpdate() {
  return expr(anyOf(
      unaryOperator(hasAnyOperatorName("++", "--"),
                    hasUnaryOperand(referencesX())),
      binaryOperator(hasOperatorName("="), hasLHS(referencesX()),
                     hasRHS(binaryOperator(
                         isAllowedBinaryOperator(),
                         hasOperands(equalsBoundNode("x"), expression())))),
      compoundAssignOperator(isAllowedCompoundOperator(), hasLHS(referencesX()),
                             hasRHS(expression()))));
}

// v = x++;
// v = x--;
// v = ++x;
// v = --x;
// v = x binop= expr;
// v = x = x binop expr;
// v = x = expr binop x;
// -----------------------
// { v = x; x binop= expr; }
// { v = x; x = x binop expr; }
// { v = x; x = expr binop x; }
// { v = x; x = expr; }
// { v = x; x++; }
// { v = x; ++x; }
// { v = x; x--; }
// { v = x; --x; }
// ----------------------
// { x = x binop expr; v = x; }
// { x = expr binop x; v = x; }
// { x binop= expr; v = x; }
// { ++x; v = x; }
// { x++; v = x; }
// { --x; v = x; }
// { x--; v = x; }
auto atomicCapture() {
  return stmt(anyOf(
      binaryOperator(hasOperatorName("="), hasRHS(atomicUpdate()),
                     hasLHS(referencesV())),
      compoundStmt(hasStatements(
          atomicRead(), binaryOperator(anyOf(atomicWrite(), atomicUpdate())))),
      compoundStmt(hasStatements(atomicUpdate(), atomicRead()))));
}

auto atomicOperation() {
  return stmt(anyOf(atomicRead().bind("read"), atomicWrite().bind("write"),
                    atomicUpdate().bind("update"),
                    atomicCapture().bind("capture")))
      .bind("operation");
}

auto atomicMatch() {
  // optionally(,
  //     optionally(ompExecutableDirective(
  //         isOpenMPParallelDirective(),
  //         hasSharedVariable(declRefExpr(to(equalsBoundNode("x")))))))

  return stmt(anyOf(atomicOperation(),
                    compoundStmt(hasSingleStatement(), has(atomicOperation()))
                        .bind("extra-compound")),
              hasAncestor(ompExecutableDirective(ompExecutableDirective(
                  isOpenMPParallelDirective(),
                  hasSharedVariable(declRefExpr(to(equalsBoundNode("x"))))))));
}

auto unnecessaryAtomic() {
  return ompAtomicDirective(
      has(atomicOperation()),
      hasAncestor(ompExecutableDirective(
          isOpenMPParallelDirective(),
          unless(hasSharedVariable(declRefExpr(to(equalsBoundNode("x"))))))));
}

} // namespace

void ReduceSynchronizationOverheadCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(ompExecutableDirective(isOpenMPParallelDirective())
                         .bind("parallel-directive"),
                     this);
  Finder->addMatcher(unnecessaryAtomic().bind("unnecessary-atomic"), this);
}

void ReduceSynchronizationOverheadCheck::check(
    const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;

  if (const auto *const Parallel =
          Result.Nodes.getNodeAs<OMPExecutableDirective>(
              "parallel-directive")) {
    const auto SharedVariables = openmp::getSharedVariables(Parallel);
    for (const ValueDecl *const Shared : SharedVariables) {
      std::vector<DiagnosticData> DiagData;
      const auto IsAReductionVariable = hasAncestor(ompExecutableDirective(
          anyOf(hasAnyClause(
                    ompReductionClause(reducesVariable(equalsNode(Shared)))),
                hasAnyClause(ompTaskReductionClause(
                    reducesVariable(equalsNode(Shared)))))));

      const auto CriticalSections = match(
          findAll(declRefExpr(
              to(equalsNode(Shared)),
              hasAncestor(ompCriticalDirective(optionally(has(atomicMatch())))
                              .bind("critical")),
              unless(IsAReductionVariable))),
          *Parallel, Ctx);

      bool FoundCriticalWithoutAtomic = false;
      for (const auto &CriticalMatch : CriticalSections) {
        const auto F = getAtomicDirective(CriticalMatch);
        if (F.Kind == AtomicKind::Unknown) {
          FoundCriticalWithoutAtomic = true;
          break;
        }
        DiagData.push_back(F);
      }

      if (FoundCriticalWithoutAtomic || DiagData.empty())
        continue;

      auto Diag = diag(Parallel->getSourceRange().getBegin(),
                       "the accesses to %0 inside critical sections can be "
                       "rewritten to use 'omp atomic's for more performance")
                  << Shared;
      for (const auto &[_, Range, Fixes] : DiagData) {
        Diag << Range << Fixes;
      }
    }

    return;
  }

  if (const auto *const UnnecessaryAtomic =
          Result.Nodes.getNodeAs<OMPAtomicDirective>("unnecessary-atomic")) {
    const auto *const Operation = Result.Nodes.getNodeAs<Stmt>("operation");
    diag(Operation->getBeginLoc(), "this operation is declared with `omp "
                                   "atomic` but it does not involve a "
                                   "shared variable")
        << Operation->getSourceRange()
        << FixItHint::CreateRemoval(UnnecessaryAtomic->getSourceRange());
  }
}

} // namespace clang::tidy::openmp
