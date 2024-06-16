//===--- UseMaskedInsteadOfDeprecatedMasterCheck.cpp - clang-tidy ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UseMaskedInsteadOfDeprecatedMasterCheck.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/Diagnostic.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPMasterDirective>
    ompMasterDirective;
// NOLINTEND(readability-identifier-naming)
} // namespace

void UseMaskedInsteadOfDeprecatedMasterCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(ompMasterDirective().bind("master"), this);
}

void UseMaskedInsteadOfDeprecatedMasterCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Master = Result.Nodes.getNodeAs<OMPMasterDirective>("master");
  diag(Master->getBeginLoc(), "the 'master' directive is deprecated since "
                              "OpenMP 5.1; use the 'masked' directive instead")
      << FixItHint::CreateReplacement(Master->getSourceRange(),
                                      "#pragma omp masked");
}

} // namespace clang::tidy::openmp
