//===--- SpecifyScheduleCheck.cpp - clang-tidy ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SpecifyScheduleCheck.h"
#include "../utils/OpenMP.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/OpenMPKinds.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/Frontend/OpenMP/OMP.h.inc"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt, OMPForDirective>
    ompForDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<
    Stmt, OMPParallelForDirective>
    ompParallelForDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<
    Stmt, OMPParallelForSimdDirective>
    ompParallelForSimdDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPScheduleClause>
    ompScheduleClause;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPOrderedClause>
    ompOrderedClause;

AST_MATCHER(OMPExecutableDirective, isOMPForDirective) {
  return isOpenMPDirectiveKind(Node.getDirectiveKind(),
                               OpenMPDirectiveKind::OMPD_for);
}

AST_MATCHER(OMPScheduleClause, isAutoKind) {
  return Node.getScheduleKind() == OpenMPScheduleClauseKind::OMPC_SCHEDULE_auto;
}

AST_MATCHER(OMPScheduleClause, isStaticWithoutChunkSize) {
  return Node.getScheduleKind() ==
             OpenMPScheduleClauseKind::OMPC_SCHEDULE_static &&
         Node.getChunkSize() == nullptr;
}
// NOLINTEND(readability-identifier-naming)
} // namespace

void SpecifyScheduleCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      ompExecutableDirective(
          isOMPForDirective(),
          anyOf(unless(ast_matchers::hasAnyClause(ompScheduleClause())),
                ast_matchers::hasAnyClause(
                    ompScheduleClause(isAutoKind()).bind("auto-schedule"))),
          optionally(
              ast_matchers::hasAnyClause(ompOrderedClause().bind("ordered"))))
          .bind("directive"),
      this);
  Finder->addMatcher(
      ompExecutableDirective(
          isOMPForDirective(),
          optionally(ast_matchers::hasAnyClause(
              ompScheduleClause(isStaticWithoutChunkSize()).bind("schedule"))),
          ast_matchers::hasAnyClause(ompOrderedClause().bind("ordered")))
          .bind("directive-ordered"),
      this);
}

void SpecifyScheduleCheck::check(const MatchFinder::MatchResult &Result) {
  if (Result.Nodes.getNodeAs<OMPExecutableDirective>("directive-ordered")) {
    const auto *Ordered = Result.Nodes.getNodeAs<OMPOrderedClause>("ordered");
    if (const auto *Schedule =
            Result.Nodes.getNodeAs<OMPScheduleClause>("schedule");
        Schedule &&
        Schedule->getScheduleKind() ==
            OpenMPScheduleClauseKind::OMPC_SCHEDULE_static &&
        !Schedule->getChunkSize()) {
      diag(
          Schedule->getBeginLoc(),
          "specify the chunk-size for the 'static' schedule to avoid executing "
          "chunks that are too large which may lead to serialized execution "
          "due to the 'ordered' clause")
          << SourceRange(Schedule->getBeginLoc(), Schedule->getEndLoc())
          << SourceRange(Ordered->getBeginLoc(), Ordered->getEndLoc());
    }
    return;
  }

  const auto *Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");

  if (const auto *Schedule =
          Result.Nodes.getNodeAs<OMPScheduleClause>("auto-schedule")) {
    diag(Schedule->getBeginLoc(),
         "the 'auto' scheduling kind results in an implementation defined "
         "schedule, which may not fit the work distribution across iterations")
        << SourceRange(Schedule->getBeginLoc(), Schedule->getEndLoc());
    return;
  }

  diag(Directive->getBeginLoc(),
       "specify the schedule for this OpenMP '%0' "
       "directive to fit the %select{|ordering dependencies and }1work "
       "distribution across iterations, "
       "the default is implementation defined")
      << Directive->getSourceRange()
      << llvm::omp::getOpenMPDirectiveName(Directive->getDirectiveKind())
      << (Result.Nodes.getNodeAs<OMPOrderedClause>("ordered") != nullptr);
}

} // namespace clang::tidy::openmp
