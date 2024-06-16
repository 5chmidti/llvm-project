//===--- MissingOrderedDirectiveAfterOrderedClauseCheck.cpp - clang-tidy --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MissingOrderedDirectiveAfterOrderedClauseCheck.h"
#include "clang/AST/Decl.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPOrderedDirective>
    ompOrderedDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPOrderedClause>
    ompOrderedClause;
// NOLINTEND(readability-identifier-naming)
} // namespace

void MissingOrderedDirectiveAfterOrderedClauseCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(ompExecutableDirective(
                         hasAnyClause(ompOrderedClause()),
                         forCallable(functionDecl().bind("context")),
                         unless(hasDescendant(ompOrderedDirective(forCallable(
                             functionDecl(equalsBoundNode("context")))))))
                         .bind("dir-with-ordered"),
                     this);
}

void MissingOrderedDirectiveAfterOrderedClauseCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Ordered =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("dir-with-ordered");

  diag(Ordered->getBeginLoc(), "this '%0' directive has an 'ordered' clause "
                               "but the associated statement "
                               "has no 'ordered' directive")
      << getOpenMPDirectiveName(Ordered->getDirectiveKind())
      << Ordered->getSourceRange();
}

} // namespace clang::tidy::openmp
