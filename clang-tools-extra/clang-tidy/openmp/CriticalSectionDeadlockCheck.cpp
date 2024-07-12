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
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Casting.h"
#include <iterator>
#include <map>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
using internal::VariadicDynCastAllOfMatcher;

// NOLINTBEGIN(readability-identifier-naming)
const VariadicDynCastAllOfMatcher<Stmt, OMPCriticalDirective>
    ompCriticalDirective;
// NOLINTEND(readability-identifier-naming)

using CallStackType = llvm::SmallVector<const CallExpr *>;
struct CriticalSectionInfo {
  CriticalSectionInfo(const OMPCriticalDirective *Critical,
                      CallStackType CallStack)
      : Critical(Critical), CallStack(std::move(CallStack)) {}

  const OMPCriticalDirective *Critical;
  CallStackType CallStack;
  friend bool operator<(const CriticalSectionInfo &Lhs,
                        const CriticalSectionInfo &Rhs) {
    if (Lhs.Critical < Rhs.Critical)
      return true;

    if (Lhs.CallStack.size() < Rhs.CallStack.size())
      return true;

    return false;
  }
};

using NodeMapType =
    std::map<CriticalSectionInfo, llvm::SmallVector<CriticalSectionInfo>>;

class NestedCriticalSectionOrderingFinder
    : public RecursiveASTVisitor<NestedCriticalSectionOrderingFinder> {
public:
  using Base = RecursiveASTVisitor<NestedCriticalSectionOrderingFinder>;
  struct BarrierInfo {
    const OMPBarrierDirective *Barrier;
    size_t IndexOfNextCritical;
  };

  explicit NestedCriticalSectionOrderingFinder(
      CriticalSectionDeadlockCheck *const Check)
      : Check(Check) {}

  bool TraverseOMPCriticalDirective(OMPCriticalDirective *const Critical) {
    if (!CriticalStack.empty()) {
      Nodes[CriticalStack.back()].emplace_back(
          Critical, DescendantCriticalDirectiveCallStack);
    }

    const DeclarationNameInfo SectionName = Critical->getDirectiveName();

    if (const auto Iter = llvm::find_if(
            CriticalStack,
            [&SectionName](const CriticalSectionInfo CriticalInPath) {
              return CriticalInPath.Critical->getDirectiveName().getName() ==
                     SectionName.getName();
            });
        Iter != CriticalStack.end()) {
      const CriticalSectionInfo FirstEntered = *Iter;
      Check->diag(FirstEntered.Critical->getBeginLoc(),
                  "deadlock while trying to enter a critical section with the "
                  "same name %0 as the already entered critical section here")
          << FirstEntered.Critical->getSourceRange() << SectionName.getName();

      size_t CallStackStart = 0U;
      if (!FirstEntered.CallStack.empty() &&
          !DescendantCriticalDirectiveCallStack.empty()) {
        CallStackStart =
            std::distance(DescendantCriticalDirectiveCallStack.begin(),
                          llvm::find(DescendantCriticalDirectiveCallStack,
                                     FirstEntered.CallStack.back()));
      }

      for (const CallExpr *const C : llvm::drop_begin(
               DescendantCriticalDirectiveCallStack, CallStackStart)) {
        Check->diag(C->getBeginLoc(), "then calls %0 here",
                    DiagnosticIDs::Level::Note)
            << C->getSourceRange() << C->getCalleeDecl()->getAsFunction();
      }

      Check->diag(Critical->getBeginLoc(),
                  "then tries to re-enter the critical section %0 here",
                  DiagnosticIDs::Level::Note)
          << Critical->getSourceRange() << SectionName.getName();
    }

    CriticalStack.emplace_back(Critical, DescendantCriticalDirectiveCallStack);

    Base::TraverseOMPCriticalDirective(Critical);

    CriticalStack.pop_back();
    return true;
  }

  bool TraverseOMPBarrierDirective(OMPBarrierDirective *const Barrier) {
    if (!CriticalStack.empty()) {
      const CriticalSectionInfo Critical = CriticalStack.back();
      const DeclarationName CriticalName =
          Critical.Critical->getDirectiveName().getName();
      Check->diag(
          Critical.Critical->getBeginLoc(),
          "barrier inside critical section %0 "
          "results in a deadlock because not all threads can reach the barrier")
          << Critical.Critical->getSourceRange() << CriticalName;

      size_t CallStackStart = 0U;
      if (!Critical.CallStack.empty() &&
          !DescendantCriticalDirectiveCallStack.empty()) {
        CallStackStart =
            std::distance(DescendantCriticalDirectiveCallStack.begin(),
                          llvm::find(DescendantCriticalDirectiveCallStack,
                                     Critical.CallStack.back()));
      }

      for (const CallExpr *const C : llvm::drop_begin(
               DescendantCriticalDirectiveCallStack, CallStackStart)) {
        Check->diag(C->getBeginLoc(), "barrier is reached by calling %0 here",
                    DiagnosticIDs::Level::Note)
            << C->getSourceRange() << C->getCalleeDecl()->getAsFunction();
      }

      Check->diag(Barrier->getBeginLoc(), "barrier encountered here",
                  DiagnosticIDs::Level::Note)
          << Barrier->getSourceRange();
    }

    return true;
  }

  bool TraverseLambdaExpr(LambdaExpr *const LE) {
    // For the declaration of a lambda, we only care about the initializers of
    // captured variables, because if they contain 'CallExpr's, then we can
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

  CallStackType DescendantCriticalDirectiveCallStack;
  llvm::SmallVector<CriticalSectionInfo> CriticalStack;

  NodeMapType Nodes;
  CriticalSectionDeadlockCheck *Check;
};
} // namespace

void CriticalSectionDeadlockCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      ompExecutableDirective(unless(hasAncestor(ompExecutableDirective())))
          .bind("directive"),
      this);
}

struct CriticalOrderEdgeType {
  CriticalOrderEdgeType(CriticalSectionInfo Source, CriticalSectionInfo Target)
      : Source(std::move(Source)), Target(std::move(Target)) {}

  CriticalSectionInfo Source;
  CriticalSectionInfo Target;
};

static void
checkInconsistentOrdering(const llvm::SmallVector<CriticalOrderEdgeType> &Edges,
                          const NodeMapType &Nodes, ClangTidyCheck *Check) {
  if (Edges.empty())
    return;

  const CriticalOrderEdgeType &LastEdge = Edges.back();

  for (const auto &[Critical, Children] : Nodes) {
    if (Critical.Critical->getDirectiveName().getName() !=
        LastEdge.Target.Critical->getDirectiveName().getName())
      continue;

    for (const CriticalSectionInfo &ChildCritical : Children) {
      llvm::SmallVector<CriticalOrderEdgeType> Tmp = Edges;
      const DeclarationNameInfo ChildCriticalName =
          ChildCritical.Critical->getDirectiveName();

      const auto CycleIter = llvm::find_if(
          Tmp, [&ChildCriticalName](const CriticalOrderEdgeType &Edge) {
            return Edge.Source.Critical->getDirectiveName().getName() ==
                   ChildCriticalName.getName();
          });
      if (CycleIter == Tmp.end()) {
        Tmp.emplace_back(Critical, ChildCritical);
        checkInconsistentOrdering(Tmp, Nodes, Check);
        continue;
      }

      const CriticalOrderEdgeType FirstCritical = *CycleIter;

      llvm::SmallVector<llvm::SmallVector<CriticalOrderEdgeType>> Orderings;
      Orderings.push_back({});
      for (const CriticalOrderEdgeType &Edge :
           llvm::drop_begin(Tmp, std::distance(Tmp.begin(), CycleIter))) {
        auto &CurrentPath = Orderings.back();

        if (CurrentPath.empty() ||
            CurrentPath.back().Target.Critical == Edge.Source.Critical)
          CurrentPath.push_back(Edge);
        else
          Orderings.push_back({Edge});
      }

      if (Orderings.back().back().Target.Critical != Critical.Critical) {
        Orderings.push_back({});
      }
      Orderings.back().emplace_back(Critical, ChildCritical);

      std::string Path;
      for (const auto &[Source, Target] : Tmp) {
        if (!Path.empty())
          Path += " -> ";
        Path += (llvm::Twine("'") +
                 Source.Critical->getDirectiveName().getAsString() + "'")
                    .str();
      }
      Path += " -> " +
              (llvm::Twine("'") +
               Critical.Critical->getDirectiveName().getAsString() + "'")
                  .str() +
              " -> " +
              (llvm::Twine("'") +
               ChildCritical.Critical->getDirectiveName().getAsString() + "'")
                  .str();

      Check->diag(Orderings.front().front().Source.Critical->getBeginLoc(),
                  "inconsistent dependency ordering for critical section %0 "
                  "can cause a deadlock; cycle involving %1 critical section "
                  "orderings detected: %2")
          << Orderings.front().front().Source.Critical->getSourceRange()
          << FirstCritical.Source.Critical->getDirectiveName().getName()
          << Orderings.size() << Path;

      for (const auto &[Index, Ordering] : llvm::enumerate(Orderings)) {
        std::string SectionPath;
        for (const auto &[Source, Target] : Ordering) {
          if (!SectionPath.empty())
            SectionPath += " -> ";
          SectionPath +=
              (llvm::Twine("'") +
               Source.Critical->getDirectiveName().getAsString() + "'")
                  .str();
        }
        SectionPath +=
            (llvm::Twine(" -> '") +
             Ordering.back().Target.Critical->getDirectiveName().getAsString() +
             "'")
                .str();

        const auto FirstSource = Ordering.front().Source;

        Check->diag(FirstSource.Critical->getBeginLoc(),
                    "Ordering %0 of %1: %2; starts here by entering %3 first",
                    DiagnosticIDs::Level::Note)
            << FirstSource.Critical->getSourceRange() << (Index + 1)
            << Orderings.size() << SectionPath
            << FirstSource.Critical->getDirectiveName().getName();

        for (const auto &[Source, Target] : Ordering) {
          size_t CallStackStart = 0U;
          if (!Source.CallStack.empty() && !Target.CallStack.empty()) {
            CallStackStart = std::distance(
                Target.CallStack.begin(),
                llvm::find(Target.CallStack, Source.CallStack.back()));
          }

          for (const auto *Call :
               llvm::drop_begin(Target.CallStack, CallStackStart))
            if (const auto *NamedCallee =
                    llvm::dyn_cast<NamedDecl>(Call->getCalleeDecl()))
              Check->diag(Call->getBeginLoc(), "then calls %0 here",
                          DiagnosticIDs::Level::Note)
                  << Call->getSourceRange() << NamedCallee;
            else
              Check->diag(Call->getBeginLoc(), "then calls function here",
                          DiagnosticIDs::Level::Note)
                  << Call->getSourceRange();

          Check->diag(Target.Critical->getBeginLoc(),
                      "then enters critical section %0 here",
                      DiagnosticIDs ::Level::Note)
              << Target.Critical->getSourceRange()
              << Target.Critical->getDirectiveName().getName();
        }
      }
    }
  }
}

void CriticalSectionDeadlockCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");

  NestedCriticalSectionOrderingFinder Visitor(this);
  Visitor.TraverseStmt(const_cast<OMPExecutableDirective *>(Directive));

  for (const auto &[Critical, Children] : Visitor.Nodes) {
    for (const auto &ChildCritical : Children)
      checkInconsistentOrdering(
          llvm::SmallVector<CriticalOrderEdgeType>{
              CriticalOrderEdgeType{Critical, ChildCritical}},
          Visitor.Nodes, this);
  }
}

} // namespace clang::tidy::openmp
