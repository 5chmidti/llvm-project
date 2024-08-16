//===--- DoNotModifyLoopVariableCheck.cpp - clang-tidy --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DoNotModifyLoopVariableCheck.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/Tooling/FixIt.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/ADT/SmallPtrSet.h"
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtOpenMP.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Analysis/Analyses/ExprMutationAnalyzer.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Casting.h>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
using internal::Matcher;
using internal::VariadicDynCastAllOfMatcher;

// NOLINTBEGIN(readability-identifier-naming)
const VariadicDynCastAllOfMatcher<Stmt, OMPLoopDirective> ompLoopDirective;
// NOLINTEND(readability-identifier-naming)

AST_MATCHER_P(OMPExecutableDirective, hasAssociatedStmt, Matcher<Stmt>,
              InnerMatcher) {
  const Stmt *const S = Node.getAssociatedStmt();
  return S && InnerMatcher.matches(*S, Finder, Builder);
}

AST_MATCHER_P(Stmt, ignoringContainers, Matcher<Stmt>, InnerMatcher) {
  return InnerMatcher.matches(*Node.IgnoreContainers(true), Finder, Builder);
}

std::optional<size_t> getOptCollapseNum(const OMPCollapseClause *const Collapse,
                                        const ASTContext &Ctx) {
  if (!Collapse)
    return std::nullopt;
  const Expr *const E = Collapse->getNumForLoops();
  const std::optional<llvm::APSInt> OptInt = E->getIntegerConstantExpr(Ctx);
  if (OptInt)
    return OptInt->tryExtValue();
  return std::nullopt;
}

class DeclRefExprInForConditionFinder
    : public RecursiveASTVisitor<DeclRefExprInForConditionFinder> {
public:
  using Base = RecursiveASTVisitor<DeclRefExprInForConditionFinder>;

  DeclRefExprInForConditionFinder(size_t NumLoopsToCheck, ASTContext &Ctx)
      : NumLoopsToCheck(NumLoopsToCheck), Ctx(Ctx) {}

  bool VisitForStmt(ForStmt *For) {
    IsInCondition = true;
    Base::TraverseStmt(For->getCond());
    IsInCondition = false;

    Base::VisitStmt(For->getBody());

    --NumLoopsToCheck;
    return NumLoopsToCheck != 0U;
  }

  bool VisitCXXForRangeStmt(CXXForRangeStmt *For) {
    --NumLoopsToCheck;
    return NumLoopsToCheck != 0U;
  }
  bool VisitDeclRefExpr(DeclRefExpr *DRef) {
    if (IsInCondition) {
      const auto *VDecl = DRef->getDecl();
      if (llvm::isa<VarDecl, FieldDecl, BindingDecl>(VDecl))
        VariablesReferencedInConditions.insert(VDecl);
    }

    return true;
  }

  bool IsInCondition = false;

  size_t NumLoopsToCheck;
  llvm::SmallPtrSet<const ValueDecl *, 4> VariablesReferencedInConditions;
  ASTContext &Ctx;
};

class AllMutationsFinder : private ExprMutationAnalyzer {
public:
  AllMutationsFinder(const Stmt &Stm, ASTContext &Context)
      : ExprMutationAnalyzer(Stm, Context), Stm(Stm), Context(Context) {}
  llvm::SmallVector<const Stmt *, 4> findAllMutations(const Decl *const Dec) {
    const auto Var = declRefExpr(to(
        // `Dec` or a binding if `Dec` is a decomposition.
        anyOf(equalsNode(Dec), bindingDecl(forDecomposition(equalsNode(Dec))))
        //
        ));

    const auto Refs =
        match(findAll(declRefExpr(
                  expr().bind("expr"), Var,
                  unless(hasParent(cxxOperatorCallExpr(
                      hasOverloadedOperatorName("[]"), hasArgument(0, Var)))),
                  unless(hasAncestor(forStmt(anyOf(
                      hasLoopInit(hasDescendant(expr(equalsBoundNode("expr")))),
                      hasIncrement(
                          hasDescendant(expr(equalsBoundNode("expr")))))))))),
              Stm, Context);

    auto Mutations = llvm::SmallVector<const Stmt *, 4>{};
    for (const auto &RefNodes : Refs) {
      const auto *const E = RefNodes.getNodeAs<Expr>("expr");
      if (isMutated(E))
        Mutations.push_back(E);
    }
    return Mutations;
  }

private:
  const Stmt &Stm;
  ASTContext &Context;
};
} // namespace

void DoNotModifyLoopVariableCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      ompLoopDirective(hasAssociatedStmt(ignoringContainers(
                           mapAnyOf(forStmt, cxxForRangeStmt)
                               .with(hasBody(stmt().bind("scope")))
                               .bind("for-like"))))
          .bind("directive"),
      this);
}

void DoNotModifyLoopVariableCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Scope = Result.Nodes.getNodeAs<Stmt>("scope");
  const auto *const LoopDirective =
      Result.Nodes.getNodeAs<OMPLoopDirective>("directive");
  const auto *const ForLike = Result.Nodes.getNodeAs<Stmt>("for-like");
  const std::optional<size_t> OptCollapseNum = getOptCollapseNum(
      LoopDirective->getSingleClause<OMPCollapseClause>(), *Result.Context);
  auto Analyzer = AllMutationsFinder(*Scope, *Result.Context);

  llvm::SmallPtrSet<const ValueDecl *, 4> Counters = {};

  for (const Expr *E : LoopDirective->counters())
    if (E)
      if (const auto *const DRef =
              llvm::dyn_cast<DeclRefExpr>(E->IgnoreImpCasts()))
        Counters.insert(DRef->getDecl());

  for (const Expr *E : LoopDirective->dependent_counters())
    if (E)
      if (const auto *const DRef =
              llvm::dyn_cast<DeclRefExpr>(E->IgnoreImpCasts()))
        Counters.insert(DRef->getDecl());

  for (const ValueDecl *Counter : Counters) {
    for (const Stmt *const Mutation : Analyzer.findAllMutations(Counter))
      diag(Mutation->getBeginLoc(),
           "do not mutate the variable '%0' used as the for statement counter "
           "of the OpenMP work-sharing construct")
          << Mutation->getSourceRange()
          << tooling::fixit::getText(*Mutation, *Result.Context);
  }

  DeclRefExprInForConditionFinder Visitor{OptCollapseNum.value_or(1U),
                                          *Result.Context};
  Visitor.TraverseStmt(const_cast<Stmt *>(ForLike));

  auto VariablesReferencedInConditions =
      llvm::set_difference(Visitor.VariablesReferencedInConditions, Counters);

  for (const ValueDecl *Var : VariablesReferencedInConditions) {
    for (const Stmt *const MutationStmt : Analyzer.findAllMutations(Var)) {
      diag(MutationStmt->getBeginLoc(),
           "do not mutate the variable %0 used in the for statement condition "
           "of the OpenMP work-sharing construct")
          << MutationStmt->getSourceRange() << Var;
    }
  }
}

} // namespace clang::tidy::openmp
