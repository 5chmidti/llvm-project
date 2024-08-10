//===--- UseAtomicInsteadOfCriticalCheck.cpp - clang-tidy -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UseAtomicInsteadOfCriticalCheck.h"
#include "../utils/Matchers.h"
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

AST_MATCHER(BinaryOperator, isAllowedOrderingOperator) {
  const auto Code = Node.getOpcode();
  return Code == BinaryOperator::Opcode::BO_LT ||
         Code == BinaryOperator::Opcode::BO_GT;
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

AST_MATCHER_P2(CompoundStmt, withOrderedSubStmt,
               ast_matchers::internal::Matcher<Stmt>, First,
               ast_matchers::internal::Matcher<Stmt>, Second) {
  if (Node.size() != 2)
    return false;

  return First.matches(*Node.body_front(), Finder, Builder) &&
         Second.matches(*Node.body_back(), Finder, Builder);
}

AST_MATCHER(QualType, isIntegral) {
  return Node->isIntegralType(Finder->getASTContext());
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

// TODO:
//
// - During the execution of an atomic region, multiple syntactic occurrences of
// expr must evaluate to the same value.
//
// - In forms that capture the original value of x in v, v and e may not refer
// to, or access, the same storage location.

auto isVXRD(bool V = true, bool X = true, bool R = true, bool D = true) {
  return varDecl(
      anyOf(V ? varDecl(equalsBoundNode("v")) : varDecl(unless(anything())),
            V ? varDecl(equalsBoundNode("x")) : varDecl(unless(anything())),
            V ? varDecl(equalsBoundNode("r")) : varDecl(unless(anything())),
            V ? varDecl(equalsBoundNode("d")) : varDecl(unless(anything()))));
}

auto declareX() {
  return declRefExpr(
      to(varDecl(unless(isVXRD(true, false, true, true))).bind("x")));
}
auto referencesX() { return declRefExpr(to(equalsBoundNode("x"))); }

auto declareV() {
  return declRefExpr(to(unless(equalsBoundNode("x")))).bind("v");
}

auto declareD() {
  return declRefExpr(
      to(varDecl(unless(isVXRD(true, true, true, false))).bind("d")));
}

auto declareE() { return declRefExpr(to(varDecl().bind("e"))); }

auto declareR() {
  return declRefExpr(
      hasType(isIntegral()),
      to(varDecl(unless(isVXRD(true, true, false, true))).bind("r")));
}
auto referenceR() { return declRefExpr(to(equalsBoundNode("r"))); }

auto declareExpression() {
  return expr(unless(declRefExpr(to(isVXRD()))),
              unless(hasDescendant(declRefExpr(to(isVXRD())))));
}
auto referenceExpression() { return declareExpression(); }

// v = x;
auto readAtomic() {
  return binaryOperator(isAssignmentOperator(),
                        unless(isAllowedCompoundOperator()), hasRHS(declareX()),
                        hasLHS(declareV()));
}

// x = expr;
auto writeAtomic() {
  return binaryOperator(isAssignmentOperator(),
                        unless(isAllowedCompoundOperator()), hasLHS(declareX()),
                        hasRHS(declareExpression()));
}

// x++;
// x--;
// ++x;
// --x;
// x binop= expr;
// x = x binop expr;
// x = expr binop x;
auto updateStmt() {
  return expr(anyOf(
      unaryOperator(hasAnyOperatorName("++", "--"),
                    hasUnaryOperand(declareX())),
      binaryOperator(hasOperatorName("="), hasLHS(declareX()),
                     hasRHS(binaryOperator(isAllowedBinaryOperator(),
                                           hasOperands(equalsBoundNode("x"),
                                                       declareExpression())))),
      compoundAssignOperator(isAllowedCompoundOperator(), hasLHS(declareX()),
                             hasRHS(declareExpression()))));
}

// x = expr ordop x ? expr : x;
// x = x ordop expr ? expr : x;
// x = x == e ? d : x;
auto conditionalUpdateAtomic() {
  return binaryOperator(
      hasOperatorName("="), hasLHS(declareX()),
      hasRHS(expr(anyOf(
          conditionalOperator(
              hasCondition(
                  binaryOperator(isAllowedOrderingOperator(),
                                 hasOperands(declareX(), declareExpression()))),
              hasTrueExpression(referenceExpression()),
              hasFalseExpression(referencesX())),
          conditionalOperator(hasCondition(binaryOperator(
                                  matchers::isEqualityOperator(),
                                  hasLHS(declareX()), hasRHS(declareE()))),
                              hasTrueExpression(declareD()),
                              hasFalseExpression(referencesX()))))));
}

auto updateAtomic() {
  return expr(anyOf(updateStmt(), conditionalUpdateAtomic()));
}

// if (expr ordop x) { x = expr; }
// if (x ordop expr) { x = expr; }
// if (x == e) { x = d; }
auto condUpdateStmt() {
  return ifStmt(anyOf(
      ifStmt(hasCondition(expr(binaryOperator(isAllowedOrderingOperator(),
                                              hasEitherOperand(declareX())))),
             has(compoundStmt(
                 has(binaryOperator(hasOperatorName("="), hasLHS(referencesX()),
                                    hasRHS(declareExpression())))))),
      ifStmt(
          hasCondition(binaryOperator(matchers::isEqualityOperator(),
                                      hasLHS(declareX()), hasRHS(declareE()))),
          has(compoundStmt(
              has(binaryOperator(hasOperatorName("="), hasLHS(referencesX()),
                                 hasRHS(declareD()))))))));
}

// FIXME: recheck all atomic definitions

// v = expr-stmt;
// { v = x; expr-stmt; }
// { expr-stmt; v = x; }
auto captureStmt() {
  const auto ExprStmt = expr(anyOf(readAtomic(), writeAtomic(), updateAtomic(),
                                   conditionalUpdateAtomic()));
  return stmt(
      anyOf(binaryOperator(hasOperatorName("="), hasRHS(ExprStmt),
                           hasLHS(declareV())),
            compoundStmt(hasStatements(readAtomic(), binaryOperator(ExprStmt))),
            compoundStmt(hasStatements(ExprStmt, readAtomic()))));
}

// { cond-update-stmt v = x; }
// if (x == e) { x = d; } else { v = x; }
// { r = x == e; if (r) { x = d; } }
// { r = x == e; if (r) { x = d; } else { v = x; } }
auto conditionalUpdateCaptureAtomic() {
  const auto XEqualsE = binaryOperator(matchers::isEqualityOperator(),
                                       hasLHS(declareX()), hasRHS(declareE()));

  const auto AssignDToX = binaryOperator(
      hasOperatorName("="), hasLHS(referencesX()), hasRHS(declareD()));

  return expr(anyOf(
      compoundStmt(withOrderedSubStmt(condUpdateStmt(), readAtomic())),
      ifStmt(ifStmt(hasCondition(XEqualsE)),
             hasThen(compoundStmt(has(AssignDToX))),
             hasElse(compoundStmt(has(readAtomic())))),
      compoundStmt(withOrderedSubStmt(
          binaryOperator(hasOperatorName("="), hasLHS(declareR()),
                         hasRHS(XEqualsE)),
          ifStmt(hasCondition(referenceR()),
                 hasThen(compoundStmt(has(AssignDToX))),
                 unless(hasElse(unless(compoundStmt(has(readAtomic()))))))))));
}

auto captureAtomic() {
  return expr(anyOf(captureStmt(), conditionalUpdateCaptureAtomic()));
}

auto atomicOperation() {
  return stmt(anyOf(readAtomic().bind("read"), writeAtomic().bind("write"),
                    updateAtomic().bind("update"),
                    captureAtomic().bind("capture")))
      .bind("operation");
}

auto atomicMatch() {
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
      hasAncestor(
          ompExecutableDirective(
              isOpenMPParallelDirective(),
              unless(hasAncestor(
                  ompExecutableDirective(isOpenMPParallelDirective()))),
              unless(hasSharedVariable(declRefExpr(to(equalsBoundNode("x"))))))
              .bind("directive")));
}

} // namespace

void UseAtomicInsteadOfCriticalCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      ompExecutableDirective(isOpenMPParallelDirective(),
                             unless(hasAncestor(ompExecutableDirective(
                                 isOpenMPParallelDirective()))))
          .bind("parallel-directive"),
      this);
  Finder->addMatcher(unnecessaryAtomic().bind("unnecessary-atomic"), this);
}

void UseAtomicInsteadOfCriticalCheck::check(
    const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;

  class Visitor : public RecursiveASTVisitor<Visitor> {
  public:
    using Base = RecursiveASTVisitor<Visitor>;

    explicit Visitor(ASTContext &Ctx, const ValueDecl *VDecl,
                     const Stmt *TargetS)
        : VDecl(VDecl), TargetS(TargetS), State(Ctx) {}

    bool TraverseOMPExecutableDirective(OMPExecutableDirective *Directive) {
      State.add(Directive);
      if (!Directive->isStandaloneDirective()) {
        Stmt *Statement = Directive->getStructuredBlock();
        Base::TraverseStmt(Statement);
      }
      State.pop();

      return true;
    }

    bool TraverseStmt(Stmt *S) {
      if (S == TargetS) {
        IsShared = State.SharedAndPrivateVars.is(
            VDecl, SharedAndPrivateState::State::Shared);
        return false;
      }

      return Base::TraverseStmt(S);
    }

    const ValueDecl *VDecl;
    const Stmt *TargetS;
    VariableState State;
    bool IsShared = false;
  };

  if (const auto *const Parallel =
          Result.Nodes.getNodeAs<OMPExecutableDirective>(
              "parallel-directive")) {
    const auto SharedVariables = openmp::getSharedVariables(Parallel, Ctx);
    for (const ValueDecl *const Shared : SharedVariables) {
      std::vector<DiagnosticData> DiagData;
      const auto IsAReductionVariable = hasAncestor(ompExecutableDirective(
          anyOf(ast_matchers::hasAnyClause(
                    ompReductionClause(reducesVariable(equalsNode(Shared)))),
                ast_matchers::hasAnyClause(ompTaskReductionClause(
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
        const auto *X = CriticalMatch.getNodeAs<ValueDecl>("x");
        const auto *Critical = CriticalMatch.getNodeAs<Stmt>("critical");

        Visitor Vis{*Result.Context, X, Critical};
        Vis.TraverseOMPExecutableDirective(
            const_cast<OMPExecutableDirective *>(Parallel));
        if (!Vis.IsShared)
          return;

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
    const auto *const Directive =
        Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");
    const auto *X = Result.Nodes.getNodeAs<ValueDecl>("x");

    Visitor Vis{*Result.Context, X, UnnecessaryAtomic};
    Vis.TraverseOMPExecutableDirective(
        const_cast<OMPExecutableDirective *>(Directive));
    if (Vis.IsShared)
      return;

    const auto *const Operation = Result.Nodes.getNodeAs<Stmt>("operation");
    diag(Operation->getBeginLoc(), "this operation is declared with `omp "
                                   "atomic` but it does not involve a "
                                   "shared variable")
        << Operation->getSourceRange()
        << FixItHint::CreateRemoval(UnnecessaryAtomic->getSourceRange());
  }
}

} // namespace clang::tidy::openmp
