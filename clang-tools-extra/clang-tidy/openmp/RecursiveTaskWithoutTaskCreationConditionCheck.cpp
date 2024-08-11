//===--- RecursiveTaskWithoutTaskCreationConditionCheck.cpp - clang-tidy --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RecursiveTaskWithoutTaskCreationConditionCheck.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "llvm/ADT/STLExtras.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPTaskDirective>
    ompTaskDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPIfClause>
    ompIfClause;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPFinalClause>
    ompFinalClause;
// NOLINTEND(readability-identifier-naming)
} // namespace

class RecursiveTaskFinder : public RecursiveASTVisitor<RecursiveTaskFinder> {
public:
  using Base = RecursiveASTVisitor<RecursiveTaskFinder>;

  RecursiveTaskFinder(ClangTidyCheck *Check, const OMPTaskDirective *Task)
      : Check(Check), Task(Task) {}

  bool TraverseOMPTaskDirective(OMPTaskDirective *TD) {
    if (TD == Task) {
      Check->diag(Task->getBeginLoc(),
                  "recursive task creation without an 'if' or 'final' clause "
                  "may degrade performance due to the task-management overhead")
          << Task->getSourceRange();
      if (!Task->getClausesOfKind<OMPDependClause>().empty())
        Check->diag(
            Task->getBeginLoc(),
            "the 'depend' clause of a task is ignored when the 'if' or 'final' "
            "clause evalutates to 'false'; ensure correctness if required",
            DiagnosticIDs::Level::Note)
            << Task->getSourceRange();

      for (const auto &[FDecl, Call] : CallStack) {
        Check->diag(Call->getBeginLoc(), "task reached by calling %0 here",
                    DiagnosticIDs::Level::Note)
            << FDecl << Call->getSourceRange();
      }

      return false;
    }

    return Base::TraverseOMPTaskDirective(TD);
  }

  bool TraverseCallExpr(CallExpr *const CE) {
    Base::TraverseCallExpr(CE);
    const auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::find_if(CallStack, [FDecl](const auto &DeclAndCallPair) {
          return DeclAndCallPair.first == FDecl;
        }) != CallStack.end())
      return true;

    CallStack.emplace_back(FDecl, CE);
    if (FDecl->hasBody())
      TraverseStmt(FDecl->getBody());
    CallStack.pop_back();
    return true;
  }

  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *const CE) {
    Base::TraverseCXXOperatorCallExpr(CE);
    const auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::find_if(CallStack, [FDecl](const auto &DeclAndCallPair) {
          return DeclAndCallPair.first == FDecl;
        }) != CallStack.end())
      return true;

    CallStack.emplace_back(FDecl, CE);
    if (FDecl->hasBody())
      TraverseStmt(FDecl->getBody());
    CallStack.pop_back();
    return true;
  }

private:
  ClangTidyCheck *Check;
  const OMPTaskDirective *Task;
  llvm::SmallVector<std::pair<const FunctionDecl *, const CallExpr *>>
      CallStack;
};

void RecursiveTaskWithoutTaskCreationConditionCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      ompTaskDirective(unless(anyOf(hasAnyClause(ompIfClause()),
                                    hasAnyClause(ompFinalClause()))))
          .bind("task"),
      this);
}

void RecursiveTaskWithoutTaskCreationConditionCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Task = Result.Nodes.getNodeAs<OMPTaskDirective>("task");
  RecursiveTaskFinder Visitor(this, Task);
  Visitor.TraverseStmt(const_cast<Stmt *>(Task->getStructuredBlock()));
}

} // namespace clang::tidy::openmp
