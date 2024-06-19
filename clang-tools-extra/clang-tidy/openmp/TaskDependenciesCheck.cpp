//===--- TaskDependenciesCheck.cpp - clang-tidy ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TaskDependenciesCheck.h"
#include "../utils/ASTUtils.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprOpenMP.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Analysis/Analyses/ExprMutationAnalyzer.h"
#include "clang/Basic/OpenMPKinds.h"
#include "clang/Basic/OperatorKinds.h"
#include "clang/Tooling/FixIt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include <type_traits>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPTaskDirective>
    ompTaskDirective;
// NOLINTEND(readability-identifier-naming)

AST_MATCHER_P(Stmt, isIdenticalTo, const Stmt *, Other) {
  return utils::areStatementsIdentical(&Node, Other, Finder->getASTContext());
}

bool isAssignmentOnlyOperator(const BinaryOperator &Op) {
  return Op.getOpcode() == BinaryOperator::Opcode::BO_Assign;
}

bool isAssignmentOnlyOperator(const CXXOperatorCallExpr &Op) {
  return Op.getOperator() == OverloadedOperatorKind::OO_Equal;
}

bool isAssignmentOnlyOperator(const CXXRewrittenBinaryOperator &Op) {
  return Op.getOpcode() == BinaryOperator::Opcode::BO_Assign;
}

AST_POLYMORPHIC_MATCHER(
    isAssignmentOnlyOperator,
    AST_POLYMORPHIC_SUPPORTED_TYPES(BinaryOperator, CXXOperatorCallExpr,
                                    CXXRewrittenBinaryOperator)) {
  return isAssignmentOnlyOperator(Node);
}

void TaskDependenciesCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(ompTaskDirective().bind("task"), this);
}

OpenMPDependClauseKind add(const OpenMPDependClauseKind CurrentGuess,
                           const OpenMPDependClauseKind New) {
  switch (CurrentGuess) {
  case OpenMPDependClauseKind::OMPC_DEPEND_unknown:
    return New;
  case OpenMPDependClauseKind::OMPC_DEPEND_in:
    switch (New) {
    case OpenMPDependClauseKind::OMPC_DEPEND_unknown:
      return OpenMPDependClauseKind::OMPC_DEPEND_in;
    case OpenMPDependClauseKind::OMPC_DEPEND_in:
      return OpenMPDependClauseKind::OMPC_DEPEND_in;
    case OpenMPDependClauseKind::OMPC_DEPEND_out:
      return OpenMPDependClauseKind::OMPC_DEPEND_inout;
    case OpenMPDependClauseKind::OMPC_DEPEND_inout:
      return OpenMPDependClauseKind::OMPC_DEPEND_inout;
    default:
      return CurrentGuess;
    }
  case OpenMPDependClauseKind::OMPC_DEPEND_out:
    switch (New) {
    case OpenMPDependClauseKind::OMPC_DEPEND_unknown:
      return OpenMPDependClauseKind::OMPC_DEPEND_out;
    case OpenMPDependClauseKind::OMPC_DEPEND_in:
      return OpenMPDependClauseKind::OMPC_DEPEND_inout;
    case OpenMPDependClauseKind::OMPC_DEPEND_out:
      return OpenMPDependClauseKind::OMPC_DEPEND_out;
    case OpenMPDependClauseKind::OMPC_DEPEND_inout:
      return OpenMPDependClauseKind::OMPC_DEPEND_inout;
    default:
      return CurrentGuess;
    }
  case OpenMPDependClauseKind::OMPC_DEPEND_inout:
    return OpenMPDependClauseKind::OMPC_DEPEND_inout;
  default:
    return CurrentGuess;
  }
}

llvm::StringRef toStringRef(const OpenMPDependClauseKind Kind) {
  switch (Kind) {
  case OMPC_DEPEND_in:
    return "in";
  case OMPC_DEPEND_out:
    return "out";
  case OMPC_DEPEND_inout:
    return "inout";
  case OMPC_DEPEND_mutexinoutset:
    return "mutexinoutset";
  case OMPC_DEPEND_depobj:
    return "depobj";
  case OMPC_DEPEND_source:
    return "source";
  case OMPC_DEPEND_sink:
    return "sink";
  case OMPC_DEPEND_inoutset:
    return "inoutset";
  case OMPC_DEPEND_outallmemory:
    return "outallmemory";
  case OMPC_DEPEND_inoutallmemory:
    return "inoutallmemory";
  case OMPC_DEPEND_unknown:
    return "unknown";
  }
}

void TaskDependenciesCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Task = Result.Nodes.getNodeAs<OMPTaskDirective>("task");
  ExprMutationAnalyzer Analyzer{*Task->getStructuredBlock(), *Result.Context};
  for (const auto *Depend : Task->getClausesOfKind<OMPDependClause>()) {
    const OpenMPDependClauseKind DependencyKind = Depend->getDependencyKind();
    if (!llvm::is_contained({OpenMPDependClauseKind::OMPC_DEPEND_in,
                             OpenMPDependClauseKind::OMPC_DEPEND_out,
                             OpenMPDependClauseKind::OMPC_DEPEND_inout},
                            DependencyKind))
      continue;

    for (const Expr *VarRef : Depend->getVarRefs()) {
      OpenMPDependClauseKind Guess =
          OpenMPDependClauseKind::OMPC_DEPEND_unknown;

      if (const auto *ArraySection =
              llvm::dyn_cast<OMPArraySectionExpr>(VarRef)) {
        VarRef = ArraySection->getBase()->IgnoreImplicit();
      }

      if (!llvm::isa<DeclRefExpr, ArraySubscriptExpr>(VarRef)) {
        VarRef->dump();
        continue;
      }

      for (const ast_matchers::BoundNodes &Match :
           match(findAll(expr(
                     isIdenticalTo(VarRef), expr().bind("expr"),
                     optionally(hasAncestor(
                         binaryOperation(
                             isAssignmentOnlyOperator(),
                             hasLHS(anyOf(
                                 expr(equalsBoundNode("expr")),
                                 hasDescendant(expr(equalsBoundNode("expr"))))))
                             .bind("assign"))))),
                 *Task->getStructuredBlock(), *Result.Context)) {
        const auto *Other = Match.getNodeAs<Expr>("expr");
        if (!Other)
          continue;

        const bool IsRead = Match.getNodeAs<Expr>("assign") == nullptr;

        Guess =
            add(Guess, Analyzer.isMutated(Other)
                           ? (IsRead ? OpenMPDependClauseKind::OMPC_DEPEND_inout
                                     : OpenMPDependClauseKind::OMPC_DEPEND_out)
                           : OpenMPDependClauseKind::OMPC_DEPEND_in);
      }

      if (DependencyKind == Guess)
        continue;

      if (Guess == OpenMPDependClauseKind::OMPC_DEPEND_inout) {
        diag(VarRef->getBeginLoc(),
             "the dependency of '%0' is under-specified; use '%1'")
            << tooling::fixit::getText(*VarRef, *Result.Context)
            << toStringRef(Guess) << VarRef->getSourceRange();
        continue;
      }

      if (DependencyKind == OpenMPDependClauseKind::OMPC_DEPEND_inout &&
          Guess < DependencyKind) {
        diag(VarRef->getBeginLoc(),
             "the dependency of '%0' is over-specified; use '%1'")
            << tooling::fixit::getText(*VarRef, *Result.Context)
            << toStringRef(Guess) << VarRef->getSourceRange();
        continue;
      }

      diag(VarRef->getBeginLoc(), "the dependency of '%0' should be '%1'")
          << tooling::fixit::getText(*VarRef, *Result.Context)
          << toStringRef(Guess) << VarRef->getSourceRange();
    }
  }
}

} // namespace clang::tidy::openmp
