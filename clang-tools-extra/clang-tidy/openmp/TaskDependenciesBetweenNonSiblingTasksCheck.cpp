//===--- TaskDependenciesBetweenNonSiblingTasksCheck.cpp - clang-tidy -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TaskDependenciesBetweenNonSiblingTasksCheck.h"
#include "../utils/OpenMP.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprOpenMP.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/DiagnosticIDs.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPDependClause>
    ompDependClause;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPArraySectionExpr>
    ompArraySectionExpr;
// NOLINTEND(readability-identifier-naming)

namespace {
AST_MATCHER_P(OMPDependClause, forEachDependency,
              ast_matchers::internal::Matcher<Expr>, InnerMatcher) {
  bool FoundMatch = false;
  ast_matchers::internal::BoundNodesTreeBuilder Result;
  for (const Expr *Dep : Node.getVarRefs()) {
    auto Tmp = *Builder;
    if (InnerMatcher.matches(*Dep, Finder, &Tmp)) {
      FoundMatch = true;
      Result.addMatch(Tmp);
    }
  }
  *Builder = std::move(Result);
  return FoundMatch;
}
} // namespace

void TaskDependenciesBetweenNonSiblingTasksCheck::registerMatchers(
    MatchFinder *Finder) {
  const auto OuterVarRef =
      declRefExpr(to(varDecl().bind("var"))).bind("outer-dref");
  const auto InnerVarRef =
      declRefExpr(to(varDecl(equalsBoundNode("var")))).bind("inner-dref");

  Finder->addMatcher(
      ompTaskDirective(
          ast_matchers::hasAnyClause(ompDependClause(forEachDependency(expr(
              anyOf(OuterVarRef,
                    ompArraySectionExpr(forEachDescendant(OuterVarRef))))))),
          forCallable(functionDecl().bind("context")),
          hasDescendant(
              ompTaskDirective(
                  ast_matchers::hasAnyClause(
                      ompDependClause(forEachDependency(expr(anyOf(
                          InnerVarRef, ompArraySectionExpr(
                                           forEachDescendant(InnerVarRef))))))),
                  forCallable(functionDecl(equalsBoundNode("context"))))
                  .bind("inner"))),
      this);
}

void TaskDependenciesBetweenNonSiblingTasksCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *OuterDRef = Result.Nodes.getNodeAs<DeclRefExpr>("outer-dref");
  const auto *InnerDRef = Result.Nodes.getNodeAs<DeclRefExpr>("inner-dref");
  const auto *Var = Result.Nodes.getNodeAs<VarDecl>("var");

  diag(InnerDRef->getBeginLoc(),
       "the dependency of %0 from a child task will not impose ordering with "
       "the dependency of its ancestors dependency")
      << InnerDRef->getSourceRange() << Var;
  diag(OuterDRef->getBeginLoc(), "%0 was specified in a 'depend' clause here",
       DiagnosticIDs::Level::Note)
      << OuterDRef->getSourceRange() << Var;
}

} // namespace clang::tidy::openmp
