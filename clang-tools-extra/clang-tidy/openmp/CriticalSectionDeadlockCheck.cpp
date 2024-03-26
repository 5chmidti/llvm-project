//===--- CriticalSectionDeadlockCheck.cpp - clang-tidy --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CriticalSectionDeadlockCheck.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/LambdaCapture.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include <algorithm>
#include <vector>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
using internal::VariadicDynCastAllOfMatcher;

// NOLINTBEGIN(readability-identifier-naming)
const VariadicDynCastAllOfMatcher<Stmt, OMPCriticalDirective>
    ompCriticalDirective;
// NOLINTEND(readability-identifier-naming)

using CallStackType = llvm::SmallVector<const CallExpr *, 4>;

class NestedCriticalSectionOrderingFinder
    : public RecursiveASTVisitor<NestedCriticalSectionOrderingFinder> {
public:
  using Base = RecursiveASTVisitor<NestedCriticalSectionOrderingFinder>;
  struct CriticalSectionInfo {
    const OMPCriticalDirective *Critical;
    CallStackType CallStack;
  };
  struct BarrierInfo {
    const OMPBarrierDirective *Barrier;
    size_t IndexOfNextCritical;
  };

  explicit NestedCriticalSectionOrderingFinder(
      CriticalSectionDeadlockCheck *const Check)
      : Check(Check) {}

  bool TraverseOMPCriticalDirective(OMPCriticalDirective *const Critical) {
    // Mark the ancestor as having at least one child
    if (!DescendantCriticalDirectiveHasChild.empty())
      DescendantCriticalDirectiveHasChild.back() = true;

    DescendantCriticalDirectiveHasChild.push_back(false);
    CurrentOrdering.push_back(
        CriticalSectionInfo{Critical, DescendantCriticalDirectiveCallStack});
    Base::TraverseOMPCriticalDirective(Critical);

    if (!DescendantCriticalDirectiveHasChild.empty() &&
        !DescendantCriticalDirectiveHasChild.back())
      Orderings.push_back(CurrentOrdering);

    CurrentOrdering.pop_back();
    DescendantCriticalDirectiveHasChild.pop_back();
    return true;
  }

  bool TraverseOMPBarrierDirective(OMPBarrierDirective *const Barrier) {
    if (!CurrentOrdering.empty()) {
      const auto &[Critical, CallStackOfCritical] = CurrentOrdering.back();
      const DeclarationName CriticalName =
          Critical->getDirectiveName().getName();
      const CallExpr *const FirstCall =
          !CallStackOfCritical.empty()
              ? CallStackOfCritical.back()
              : DescendantCriticalDirectiveCallStack.front();
      Check->diag(
          FirstCall->getBeginLoc(),
          "barrier inside critical section %1 "
          "results in a deadlock because not all threads can reach the barrier")
          << FirstCall->getSourceRange()
          << FirstCall->getCalleeDecl()->getAsFunction() << CriticalName;
      Check->diag(Critical->getBeginLoc(),
                  "barrier encountered while inside critical section %0 here",
                  DiagnosticIDs::Level::Note)
          << Critical->getSourceRange() << CriticalName;
      for (const CallExpr *const C : DescendantCriticalDirectiveCallStack) {
        Check->diag(C->getBeginLoc(), "barrier is reached by calling %0 here",
                    DiagnosticIDs::Level::Note)
            << C->getSourceRange() << C->getCalleeDecl()->getAsFunction();
      }
    }

    return true;
  }

  bool TraverseLambdaExpr(LambdaExpr *const LE) {
    // For the declaration of a lambda, we only care about the initializers of
    // captured variables, because if they contain 'CallExpr's, the we can
    // detect possible issues.
    for (const LambdaCapture Capture : LE->explicit_captures()) {
      if (auto *const Var =
              llvm::dyn_cast_or_null<VarDecl>(Capture.getCapturedVar());
          Var && Var->hasInit())
        Base::TraverseStmt(Var->getInit());
    }
    return true;
  }

  bool TraverseFunctionDecl(FunctionDecl *const) { return true; }

  bool TraverseCallExpr(CallExpr *const CE) {
    if (const auto *const FDecl =
            llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
        FDecl && FDecl->hasBody()) {
      if (llvm::find(DescendantCriticalDirectiveCallStack, CE) !=
          DescendantCriticalDirectiveCallStack.end())
        return true;
      DescendantCriticalDirectiveCallStack.push_back(CE);
      TraverseStmt(FDecl->getBody());
      DescendantCriticalDirectiveCallStack.pop_back();
    }
    return true;
  }

  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *const CE) {
    if (const auto *const FDecl =
            llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
        FDecl && FDecl->hasBody()) {
      if (llvm::find(DescendantCriticalDirectiveCallStack, CE) !=
          DescendantCriticalDirectiveCallStack.end())
        return true;
      DescendantCriticalDirectiveCallStack.push_back(CE);
      TraverseStmt(FDecl->getBody());
      DescendantCriticalDirectiveCallStack.pop_back();
    }
    return true;
  }

  llvm::SmallVector<bool, 4> DescendantCriticalDirectiveHasChild;
  CallStackType DescendantCriticalDirectiveCallStack;

  std::vector<llvm::SmallVector<CriticalSectionInfo, 4>> Orderings;
  llvm::SmallVector<CriticalSectionInfo, 4> CurrentOrdering;

  CriticalSectionDeadlockCheck *Check;
};

auto createCriticalEqualsNamePredicate(const DeclarationName Name) {
  return [Name](const NestedCriticalSectionOrderingFinder::CriticalSectionInfo
                    &Info) {
    return Info.Critical->getDirectiveName().getName() == Name;
  };
}
} // namespace

void CriticalSectionDeadlockCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      ompExecutableDirective(unless(hasAncestor(ompExecutableDirective())))
          .bind("directive"),
      this);
}

void CriticalSectionDeadlockCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");

  NestedCriticalSectionOrderingFinder Visitor(this);
  Visitor.TraverseStmt(const_cast<OMPExecutableDirective *>(Directive));

  const auto PrintCallStackDiags = [this](const CallStackType &CallStack,
                                          const DeclarationName Name) {
    for (const CallExpr *const CE : CallStack)
      diag(CE->getBeginLoc(),
           "critical section %0 is reached by calling %1 here",
           DiagnosticIDs::Level::Note)
          << CE->getSourceRange() << Name
          << CE->getCalleeDecl()->getAsFunction();
  };

  for (const auto &[OuterOrderIndex, Order1] :
       llvm::enumerate(Visitor.Orderings))
    for (const auto &[Order1BeforeIndex, Order1BeforeInfo] :
         llvm::enumerate(Order1)) {
      const auto &[Order1Before, Order1BeforeCallStack] = Order1BeforeInfo;
      const DeclarationName Order1BeforeName =
          Order1Before->getDirectiveName().getName();
      for (const auto &[InnerCyclicCritical, InnerCyclicCriticalCallStack] :
           llvm::ArrayRef(Order1).drop_front(Order1BeforeIndex))
        if (InnerCyclicCritical != Order1Before &&
            Order1BeforeName ==
                InnerCyclicCritical->getDirectiveName().getName()) {
          diag(Order1Before->getBeginLoc(),
               "deadlock while trying to enter a critical section with the "
               "same name %0 as the already entered critical section here")
              << Order1Before->getSourceRange() << Order1BeforeName;
          diag(InnerCyclicCritical->getBeginLoc(),
               "trying to re-enter the critical section %0 here",
               DiagnosticIDs::Level::Note)
              << InnerCyclicCritical->getSourceRange() << Order1BeforeName;
          for (const CallExpr *const CE : InnerCyclicCriticalCallStack)
            diag(CE->getBeginLoc(),
                 "critical section %0 is reached by calling %1 here",
                 DiagnosticIDs::Level::Note)
                << CE->getSourceRange()
                << InnerCyclicCritical->getDirectiveName().getName()
                << CE->getCalleeDecl()->getAsFunction();
        }
      for (size_t InnerOrderIndex = OuterOrderIndex + 1;
           InnerOrderIndex < Visitor.Orderings.size(); ++InnerOrderIndex) {
        const auto &Order2 = Visitor.Orderings[InnerOrderIndex];
        for (const auto &[Order1After, Order1AfterCallStack] :
             llvm::ArrayRef(Order1).drop_front(Order1BeforeIndex + 1)) {
          const auto *const IterOrder2 = llvm::find_if(
              Order2, createCriticalEqualsNamePredicate(Order1BeforeName));
          if (IterOrder2 == Order2.end())
            continue;
          const DeclarationName Order1AfterName =
              Order1After->getDirectiveName().getName();
          const auto *const OrderViolationIter =
              std::find_if(Order2.begin(), IterOrder2,
                           createCriticalEqualsNamePredicate(Order1AfterName));
          if (OrderViolationIter != IterOrder2) {
            diag(
                Order1Before->getBeginLoc(),
                "deadlock by inconsistent ordering of critical sections %0 and "
                "%1")
                << Order1Before->getSourceRange() << Order1BeforeName
                << Order1AfterName;

            const auto &[Order2Before, Order2BeforeCallStack] =
                *OrderViolationIter;
            const auto &[Order2After, Order2AfterCallStack] = *IterOrder2;

            diag(Order1Before->getBeginLoc(),
                 "critical section %0 is entered first in the first section "
                 "ordering here",
                 DiagnosticIDs::Level::Note)
                << Order1Before->getSourceRange() << Order1BeforeName;
            PrintCallStackDiags(Order1BeforeCallStack, Order1BeforeName);
            diag(Order1After->getBeginLoc(),
                 "critical section %0 is nested inside %1 in the first section "
                 "ordering here",
                 DiagnosticIDs::Level::Note)
                << Order1After->getSourceRange() << Order1AfterName
                << Order1BeforeName;
            PrintCallStackDiags(Order1AfterCallStack, Order1AfterName);

            const DeclarationName Order2BeforeName =
                Order2Before->getDirectiveName().getName();
            diag(Order2Before->getBeginLoc(),
                 "critical section %0 is entered first in the second section "
                 "ordering here",
                 DiagnosticIDs::Level::Note)
                << Order2Before->getSourceRange() << Order2BeforeName;
            PrintCallStackDiags(Order2BeforeCallStack, Order2BeforeName);

            const DeclarationName Order2AfterName =
                Order2After->getDirectiveName().getName();
            diag(
                Order2After->getBeginLoc(),
                "critical section %0 is nested inside %1 in the second section "
                "ordering here",
                DiagnosticIDs::Level::Note)
                << Order2After->getSourceRange() << Order2AfterName
                << Order2BeforeName;
            PrintCallStackDiags(Order2AfterCallStack, Order2AfterName);
          }
        }
      }
    }
}

} // namespace clang::tidy::openmp
