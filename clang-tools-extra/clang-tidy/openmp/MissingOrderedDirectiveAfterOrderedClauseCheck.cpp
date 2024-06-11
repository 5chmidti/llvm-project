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
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/ADT/SmallVector.h"

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
  Finder->addMatcher(
      ompExecutableDirective(
          hasAnyClause(ompOrderedClause()),
          hasAncestor(functionDecl().bind("context")),
          unless(hasDescendant(ompOrderedDirective(
              hasAncestor(functionDecl().bind("other")),
              hasAncestor(functionDecl(equalsBoundNode("context"),
                                       equalsBoundNode("other")))))))
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
