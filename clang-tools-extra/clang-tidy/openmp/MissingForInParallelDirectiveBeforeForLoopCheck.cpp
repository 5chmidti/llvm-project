//===--- MissingForInParallelDirectiveBeforeForLoopCheck.cpp - clang-tidy -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MissingForInParallelDirectiveBeforeForLoopCheck.h"
#include "../utils/LexerUtils.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include <clang/AST/ASTFwd.h>
#include <clang/AST/StmtOpenMP.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Token.h>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const internal::VariadicDynCastAllOfMatcher<Stmt, OMPParallelDirective>
    ompParallelDirective;
// NOLINTEND(readability-identifier-naming)

bool isParallel(const Token &Tok) {
  if (Tok.getKind() != tok::TokenKind::raw_identifier)
    return false;
  return Tok.getRawIdentifier() == "parallel";
}

SourceLocation getEndOfParallelWord(const SourceLocation Loc,
                                    const SourceManager &SM,
                                    const LangOptions &LO) {
  auto NextToken = utils::lexer::findNextTokenSkippingComments(Loc, SM, LO);
  while (NextToken && !isParallel(*NextToken)) {
    NextToken = utils::lexer::findNextTokenSkippingComments(
        NextToken->getEndLoc(), SM, LO);
  }

  return NextToken->getEndLoc();
}
} // namespace

void MissingForInParallelDirectiveBeforeForLoopCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      ompParallelDirective(hasStructuredBlock(forStmt())).bind("omp"), this);
}

void MissingForInParallelDirectiveBeforeForLoopCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Parallel = Result.Nodes.getNodeAs<OMPParallelDirective>("omp");
  diag(Parallel->getBeginLoc(), "OpenMP `parallel` directive does not "
                                "contain work-sharing construct `for`, but "
                                "for loop is the next statement")
      << Parallel->getSourceRange()
      << FixItHint::CreateInsertion(
             getEndOfParallelWord(Parallel->getBeginLoc(),
                                  *Result.SourceManager,
                                  Result.Context->getLangOpts()),
             " for");
}

} // namespace clang::tidy::openmp
