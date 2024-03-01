//===--- SpecifyScheduleCheck.cpp - clang-tidy ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SpecifyScheduleCheck.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/Frontend/OpenMP/OMP.h.inc"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const internal::VariadicDynCastAllOfMatcher<Stmt, OMPForDirective>
    ompForDirective;
const internal::VariadicDynCastAllOfMatcher<Stmt, OMPParallelForDirective>
    ompParallelForDirective;
const internal::VariadicDynCastAllOfMatcher<Stmt, OMPParallelForSimdDirective>
    ompParallelForSimdDirective;
const internal::VariadicDynCastAllOfMatcher<OMPClause, OMPScheduleClause>
    ompScheduleClause;
// NOLINTEND(readability-identifier-naming)
} // namespace

void SpecifyScheduleCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(mapAnyOf(ompForDirective, ompParallelForDirective,
                              ompParallelForSimdDirective)
                         .with(unless(hasAnyClause(ompScheduleClause())))
                         .bind("directive"),
                     this);
}

void SpecifyScheduleCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");
  diag(Directive->getBeginLoc(),
       "specify the schedule for this openmp '%0' "
       "directive to fit the work distribution across iterations, "
       "the default is implementation defined")
      << Directive->getSourceRange() <<
      // Directive->getStmtClassName()
      llvm::omp::getOpenMPDirectiveName(Directive->getDirectiveKind());
}

} // namespace clang::tidy::openmp
