//===--- DoNotSetNumThreadsInParallelRegionCheck.cpp - clang-tidy ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DoNotSetNumThreadsInParallelRegionCheck.h"
#include "clang/AST/Expr.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/OpenMPKinds.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
AST_MATCHER(OMPExecutableDirective, isParallelDirective) {
  return isOpenMPParallelDirective(Node.getDirectiveKind());
}
} // namespace

void DoNotSetNumThreadsInParallelRegionCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      ompExecutableDirective(
          isParallelDirective(), forCallable(functionDecl().bind("context")),
          hasDescendant(
              callExpr(callee(functionDecl(hasName("::omp_set_num_threads"))),
                       forCallable(functionDecl(equalsBoundNode("context"))))
                  .bind("call")),
          unless(hasDescendant(ompExecutableDirective(
              isParallelDirective(),
              forCallable(functionDecl(equalsBoundNode("context")))))))
          .bind("parallel"),
      this);
}

void DoNotSetNumThreadsInParallelRegionCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("parallel");
  const auto *const Call = Result.Nodes.getNodeAs<CallExpr>("call");
  diag(Call->getBeginLoc(),
       "do not call 'omp_set_num_threads' inside this parallel region, it will "
       "only affect newly entered parallel regions")
      << Call->getSourceRange() << Directive->getSourceRange();
}

} // namespace clang::tidy::openmp
