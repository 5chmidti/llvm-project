//===--- UseReductionCheck.cpp - clang-tidy -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UseReductionCheck.h"
#include "../utils/OpenMP.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/OpenMPKinds.h"
#include "clang/Basic/OperatorKinds.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
enum class ReductionKind {
  Invalid,
  Add,
  Mul,
  LAnd,
  LOr,
  Xor,
  And,
  Or,
};

llvm::StringRef toString(const ReductionKind Kind) {
  switch (Kind) {
  case ReductionKind::Invalid:
    return "Invalid";
  case ReductionKind::Add:
    return "+";
  case ReductionKind::Mul:
    return "*";
  case ReductionKind::LAnd:
    return "&&";
  case ReductionKind::LOr:
    return "||";
  case ReductionKind::Xor:
    return "^";
  case ReductionKind::And:
    return "&";
  case ReductionKind::Or:
    return "&";
  }
}

static ReductionKind translateToReductionKind(const BinaryOperatorKind Kind) {
  switch (Kind) {
  case BinaryOperatorKind::BO_Add:
    return ReductionKind::Add;
  case BinaryOperatorKind::BO_AddAssign:
    return ReductionKind::Add;

  case BinaryOperatorKind::BO_Mul:
    return ReductionKind::Mul;
  case BinaryOperatorKind::BO_MulAssign:
    return ReductionKind::Mul;

  case BinaryOperatorKind::BO_LAnd:
    return ReductionKind::LOr;
  case BinaryOperatorKind::BO_LOr:
    return ReductionKind::LOr;

  case BinaryOperatorKind::BO_Xor:
    return ReductionKind::Xor;

  case BinaryOperatorKind::BO_And:
    return ReductionKind::And;
  case BinaryOperatorKind::BO_AndAssign:
    return ReductionKind::And;

  case BinaryOperatorKind::BO_Or:
    return ReductionKind::Or;
  case BinaryOperatorKind::BO_OrAssign:
    return ReductionKind::Or;

  default:
    return ReductionKind::Invalid;
  }
}

static ReductionKind
translateToReductionKind(const OverloadedOperatorKind Kind) {
  switch (Kind) {
  case OverloadedOperatorKind::OO_Plus:
    return ReductionKind::Add;
  case OverloadedOperatorKind::OO_PlusEqual:
    return ReductionKind::Add;

  case OverloadedOperatorKind::OO_Star:
    return ReductionKind::Mul;
  case OverloadedOperatorKind::OO_StarEqual:
    return ReductionKind::Mul;

  case OverloadedOperatorKind::OO_AmpAmp:
    return ReductionKind::LAnd;
  case OverloadedOperatorKind::OO_PipePipe:
    return ReductionKind::LOr;

  case OverloadedOperatorKind::OO_Caret:
    return ReductionKind::Xor;

  case OverloadedOperatorKind::OO_Amp:
    return ReductionKind::And;
  case OverloadedOperatorKind::OO_AmpEqual:
    return ReductionKind::And;

  case OverloadedOperatorKind::OO_Pipe:
    return ReductionKind::Or;
  case OverloadedOperatorKind::OO_PipeEqual:
    return ReductionKind::Or;

  default:
    return ReductionKind::Invalid;
  }
}

static bool isCompoundReductionOperator(const BinaryOperatorKind Kind) {
  switch (Kind) {
  case BinaryOperatorKind::BO_AddAssign:
    return true;
  case BinaryOperatorKind::BO_MulAssign:
    return true;
  case BinaryOperatorKind::BO_AndAssign:
    return true;
  case BinaryOperatorKind::BO_OrAssign:
    return true;
  default:
    return false;
  }
}

static bool isCompoundReductionOperator(const OverloadedOperatorKind Kind) {
  switch (Kind) {
  case OverloadedOperatorKind::OO_PlusEqual:
    return true;
  case OverloadedOperatorKind::OO_StarEqual:
    return true;
  case OverloadedOperatorKind::OO_AmpEqual:
    return true;
  case OverloadedOperatorKind::OO_PipeEqual:
    return true;

  default:
    return false;
  }
}

static bool isReductionOperator(const BinaryOperatorKind Kind) {
  return translateToReductionKind(Kind) != ReductionKind::Invalid;
}

static bool isReductionOperator(const OverloadedOperatorKind Kind) {
  return translateToReductionKind(Kind) != ReductionKind::Invalid;
}

static ReductionKind translateToReductionKind(const Expr *Operation) {
  if (const auto *Bin = llvm::dyn_cast<BinaryOperator>(Operation)) {
    return translateToReductionKind(Bin->getOpcode());
  }
  if (const auto *Op = llvm::dyn_cast<CXXOperatorCallExpr>(Operation)) {
    return translateToReductionKind(Op->getOperator());
  }
  if (const auto *Rewritten =
          llvm::dyn_cast<CXXRewrittenBinaryOperator>(Operation)) {
    return translateToReductionKind(Rewritten->getOperator());
  }

  return ReductionKind::Invalid;
}

AST_MATCHER(Expr, isReductionOperator) {
  if (const auto *Bin = llvm::dyn_cast<BinaryOperator>(&Node)) {
    return isReductionOperator(Bin->getOpcode());
  }
  if (const auto *Op = llvm::dyn_cast<CXXOperatorCallExpr>(&Node)) {
    return isReductionOperator(Op->getOperator());
  }
  if (const auto *Rewritten =
          llvm::dyn_cast<CXXRewrittenBinaryOperator>(&Node)) {
    return isReductionOperator(Rewritten->getOperator());
  }

  return false;
}

AST_MATCHER(Expr, isCompoundReductionOperator) {
  if (const auto *Bin = llvm::dyn_cast<BinaryOperator>(&Node)) {
    return isCompoundReductionOperator(Bin->getOpcode());
  }
  if (const auto *Op = llvm::dyn_cast<CXXOperatorCallExpr>(&Node)) {
    return isCompoundReductionOperator(Op->getOperator());
  }
  if (const auto *Rewritten =
          llvm::dyn_cast<CXXRewrittenBinaryOperator>(&Node)) {
    return isCompoundReductionOperator(Rewritten->getOperator());
  }

  return false;
}

AST_MATCHER(OMPExecutableDirective, isWorksharingDirective) {
  return isOpenMPWorksharingDirective(Node.getDirectiveKind());
}
AST_MATCHER(OMPExecutableDirective, isParallelDirective) {
  return isOpenMPParallelDirective(Node.getDirectiveKind());
}
AST_MATCHER(OMPExecutableDirective, isLoopDirective) {
  return isOpenMPLoopDirective(Node.getDirectiveKind());
}

AST_MATCHER_P(DeclRefExpr, equalsAnyNode, llvm::ArrayRef<const DeclRefExpr *>,
              Nodes) {
  return llvm::is_contained(Nodes, &Node);
}

void UseReductionCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      ompExecutableDirective(isLoopDirective(), anyOf(isWorksharingDirective(),
                                                      isParallelDirective()))
          .bind("directive"),
      this);
}

void UseReductionCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");

  ASTContext &Ctx = *Result.Context;

  struct ReductionInfo {
    llvm::SmallSet<ReductionKind, 4> Reductions;
    llvm::SmallVector<const DeclRefExpr *> References;
    llvm::SmallSet<const OMPExecutableDirective *, 2> AccessDirectives;
  };

  std::map<const ValueDecl *, ReductionInfo> Reductions{};

  for (const ast_matchers::BoundNodes &M : match(
           findAll(traverse(
               TK_IgnoreUnlessSpelledInSource,
               mapAnyOf(ompAtomicDirective, ompCriticalDirective)
                   .with(has(binaryOperation(anyOf(
                       binaryOperation(
                           isAssignmentOperator(),
                           hasLHS(declRefExpr(to(varDecl().bind("var")))
                                      .bind("dref")),
                           hasRHS(
                               binaryOperation(
                                   isReductionOperator(),
                                   unless(isCompoundReductionOperator()),
                                   hasEitherOperand(
                                       declRefExpr(
                                           to(varDecl(equalsBoundNode("var"))))
                                           .bind("rhs-dref")))
                                   .bind("reduction"))),
                       binaryOperation(
                           isCompoundReductionOperator(),
                           hasLHS(declRefExpr(to(varDecl().bind("var")))
                                      .bind("dref")))
                           .bind("reduction")))))
                   .bind("access-directive"))),
           *Directive->getStructuredBlock(), Ctx)) {
    const auto *DRef = M.getNodeAs<DeclRefExpr>("dref");
    const auto *RhsDRef = M.getNodeAs<DeclRefExpr>("rhs-dref");
    const auto *Var = M.getNodeAs<VarDecl>("var");
    const auto *Op = M.getNodeAs<Expr>("reduction");

    auto &Info = Reductions[Var];
    Info.Reductions.insert(translateToReductionKind(Op));
    Info.References.push_back(DRef);
    if (RhsDRef)
      Info.References.push_back(RhsDRef);

    // When a reduction is inside an atomic directive, then the other variable
    // inside the atomic cannot be shared as per the atomic definition (access
    // to ONE storage location is atomic). Therefore, we save the atomic
    // directive for removal with a fix-it hint.
    Info.AccessDirectives.insert(
        M.getNodeAs<OMPExecutableDirective>("access-directive"));
  }

  llvm::SmallVector<const ValueDecl *> ToRemove;

  for (const auto &[Var, Info] : Reductions)
    if (!match(findAll(declRefExpr(to(varDecl(equalsNode(Var))),
                                   unless(equalsAnyNode(Info.References)))
                           .bind("dref")),
               *Directive->getStructuredBlock(), Ctx)
             .empty())
      ToRemove.push_back(Var);

  for (const ValueDecl *Var : ToRemove)
    Reductions.erase(Var);

  for (const auto &[Var, Info] : Reductions) {
    const ReductionKind Reduction = *Info.Reductions.begin();
    if (Reduction == ReductionKind::Invalid || Info.Reductions.size() != 1 ||
        Info.AccessDirectives.size() != 1)
      continue;

    const OMPExecutableDirective *AccessDirective =
        *Info.AccessDirectives.begin();
    const bool IsInAtomic = llvm::isa<OMPAtomicDirective>(AccessDirective);

    llvm::SmallVector<FixItHint, 2> Fixes{FixItHint::CreateInsertion(
        Directive->getEndLoc(), (" reduction(" + toString(Reduction) + " : " +
                                 Var->getNameAsString() + ")")
                                    .str())};

    if (IsInAtomic)
      Fixes.push_back(
          FixItHint::CreateRemoval(AccessDirective->getSourceRange()));

    {
      auto Diag = diag(Directive->getBeginLoc(),
                       "prefer to use a 'reduction(%0 : %1)' clause instead of "
                       "explicit synchronization for each update")
                  << Directive->getSourceRange() << toString(Reduction)
                  << Var->getName();
      if (IsInAtomic)
        Diag << Fixes;
    }

    if (!IsInAtomic)
      diag(AccessDirective->getBeginLoc(),
           "the reduction involves a 'critical' section, check if it can be "
           "removed in favor of a 'reduction' clause",
           DiagnosticIDs::Level::Note)
          << AccessDirective->getSourceRange() << Fixes;

    for (const DeclRefExpr *Ref : Info.References)
      diag(Ref->getLocation(), "reduction of %0 happens here",
           DiagnosticIDs::Level::Note)
          << Ref->getDecl() << Ref->getSourceRange();
  }
}

} // namespace clang::tidy::openmp
