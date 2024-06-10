//===--- ImplementationDefinedUnrollCheck.cpp - clang-tidy ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ImplementationDefinedUnrollCheck.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/SourceLocation.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPUnrollDirective>
    ompUnrollDirective;

const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPPartialClause>
    ompPartialClause;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPFullClause>
    ompFullClause;

AST_MATCHER(OMPPartialClause, hasUnrollFactor) { return Node.getFactor(); }

void ImplementationDefinedUnrollCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      ompUnrollDirective(unless(hasAnyClause(ompFullClause())),
                         unless(hasAnyClause(ompPartialClause())))
          .bind("unroll"),
      this);
  Finder->addMatcher(
      ompUnrollDirective(hasAnyClause(
          ompPartialClause(unless(hasUnrollFactor())).bind("unset-partial"))),
      this);
}

void ImplementationDefinedUnrollCheck::check(
    const MatchFinder::MatchResult &Result) {
  if (const auto *Partial =
          Result.Nodes.getNodeAs<OMPPartialClause>("unset-partial")) {
    diag(Partial->getBeginLoc(),
         "the implicit unroll factor of the 'partial' unroll clause is "
         "implementation defined")
        << SourceRange(Partial->getBeginLoc(), Partial->getEndLoc());
    return;
  }

  const auto *Unroll = Result.Nodes.getNodeAs<OMPUnrollDirective>("unroll");
  diag(Unroll->getBeginLoc(),
       "not specifying a 'partial' or 'full' clause "
       "will result in implementation defined behavior for the unrolling")
      << SourceRange(Unroll->getBeginLoc(), Unroll->getEndLoc());
}

} // namespace clang::tidy::openmp
