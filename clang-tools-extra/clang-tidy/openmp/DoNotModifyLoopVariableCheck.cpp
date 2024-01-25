//===--- DoNotModifyLoopVariableCheck.cpp - clang-tidy --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DoNotModifyLoopVariableCheck.h"
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
const VariadicDynCastAllOfMatcher<Stmt, OMPLoopBasedDirective>
    ompLoopBasedDirective;
// NOLINTEND(readability-identifier-naming)

AST_MATCHER_P(OMPExecutableDirective, hasAssociatedStmt, Matcher<Stmt>,
              InnerMatcher) {
  const Stmt *const S = Node.getAssociatedStmt();
  return S && InnerMatcher.matches(*S, Finder, Builder);
}

AST_MATCHER_P(Stmt, ignoringContainers, Matcher<Stmt>, InnerMatcher) {
  return InnerMatcher.matches(*Node.IgnoreContainers(true), Finder, Builder);
}

class DeclRefExprFinder : public RecursiveASTVisitor<DeclRefExprFinder> {
public:
  bool VisitDeclRefExpr(const DeclRefExpr *const DRef) {
    ReferencedVariables[DRef->getDecl()].push_back(DRef);
    return true;
  }

  llvm::SmallMapVector<const ValueDecl *,
                       llvm::SmallVector<const DeclRefExpr *, 4>, 4>
      ReferencedVariables{};
};

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

class IterVarsOfRawForStmtsFromCollapsedLoopDirective
    : public RecursiveASTVisitor<
          IterVarsOfRawForStmtsFromCollapsedLoopDirective> {
public:
  IterVarsOfRawForStmtsFromCollapsedLoopDirective(size_t NumLoopsToCheck,
                                                  ASTContext &Ctx)
      : NumLoopsToCheck(NumLoopsToCheck), Ctx(Ctx) {}

  bool VisitForStmt(ForStmt *For) {
    const auto MatchResult = match(LoopIterVar, *For->getInit(), Ctx);

    // Keep the current for-loop iteration variable to remove from variables
    // referenced in the current loop condition.
    llvm::SmallVector<const ValueDecl *, 4> CurrentIterLoopVars{};
    for (const auto &V : MatchResult)
      CurrentIterLoopVars.push_back(V.getNodeAs<VarDecl>("var"));
    IterLoopVars.append(CurrentIterLoopVars);

    DeclRefExprFinder DRefFinder{};
    DRefFinder.TraverseStmt(For->getCond());
    for (const auto &DeclAndRefs : DRefFinder.ReferencedVariables)
      if (!llvm::is_contained(CurrentIterLoopVars, DeclAndRefs.first))
        IterConditionVars.insert(DeclAndRefs);

    --NumLoopsToCheck;
    return NumLoopsToCheck != 0U;
  }

  bool VisitCXXForRangeStmt(CXXForRangeStmt *For) {
    --NumLoopsToCheck;
    return NumLoopsToCheck != 0U;
  }

  size_t NumLoopsToCheck;
  llvm::SmallVector<const ValueDecl *, 4> IterLoopVars{};
  llvm::SmallMapVector<const ValueDecl *,
                       llvm::SmallVector<const DeclRefExpr *, 4>, 4>
      IterConditionVars{};
  ASTContext &Ctx;
  ast_matchers::internal::BindableMatcher<Stmt> LoopIterVar = stmt(
      anyOf(declStmt(has(varDecl().bind("var"))),
            binaryOperator(isAssignmentOperator(),
                           hasLHS(declRefExpr(to(varDecl().bind("var")))))));
};

class AllMutationsFinder : private ExprMutationAnalyzer {
public:
  AllMutationsFinder(const Stmt &Stm, ASTContext &Context)
      : ExprMutationAnalyzer(Stm, Context), Stm(Stm), Context(Context) {}
  llvm::SmallVector<const Stmt *, 4> findAllMutations(const Decl *const Dec) {
    const auto Refs = match(
        findAll(declRefExpr(
                    to(
                        // `Dec` or a binding if `Dec` is a decomposition.
                        anyOf(equalsNode(Dec),
                              bindingDecl(forDecomposition(equalsNode(Dec))))
                        //
                        ))
                    .bind("expr")),
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
  Finder->addMatcher(ompLoopBasedDirective(hasAssociatedStmt(ignoringContainers(
                         mapAnyOf(forStmt, cxxForRangeStmt)
                             .with(hasBody(stmt().bind("scope")))
                             .bind("for-like")))),
                     this);
}

void DoNotModifyLoopVariableCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Scope = Result.Nodes.getNodeAs<Stmt>("scope");
  const auto *const Collapse =
      Result.Nodes.getNodeAs<OMPCollapseClause>("collapse");
  const auto *const ForLike = Result.Nodes.getNodeAs<Stmt>("for-like");
  const std::optional<size_t> OptCollapseNum =
      getOptCollapseNum(Collapse, *Result.Context);

  IterVarsOfRawForStmtsFromCollapsedLoopDirective Visitor{
      OptCollapseNum.value_or(1U), *Result.Context};
  Visitor.TraverseStmt(const_cast<Stmt *>(ForLike));

  auto Analyzer = AllMutationsFinder(*Scope, *Result.Context);
  for (const ValueDecl *const Var : Visitor.IterLoopVars) {
    for (const auto *const MutationStmt : Analyzer.findAllMutations(Var))
      diag(MutationStmt->getBeginLoc(),
           "do not mutate the variable %0 used as the for statement counter "
           "of the OpenMP work-sharing construct")
          << MutationStmt->getSourceRange() << Var;
  }

  for (const auto &[ValDecl, Refs] : Visitor.IterConditionVars) {
    for (const auto *const MutationStmt : Analyzer.findAllMutations(ValDecl)) {
      diag(MutationStmt->getBeginLoc(),
           "do not mutate the variable %0 used in the for statement condition "
           "of the OpenMP work-sharing construct")
          << MutationStmt->getSourceRange() << ValDecl;
      for (const DeclRefExpr *const ConditionVarRef : Refs)
        diag(ConditionVarRef->getLocation(),
             "variable %0 used in loop condition here",
             DiagnosticIDs::Level::Note)
            << ConditionVarRef->getSourceRange() << ValDecl;
    }
  }
}

} // namespace clang::tidy::openmp
