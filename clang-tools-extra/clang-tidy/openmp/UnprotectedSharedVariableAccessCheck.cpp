//===--- UnprotectedSharedVariableAccessCheck.cpp - clang-tidy ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UnprotectedSharedVariableAccessCheck.h"
#include "../utils/Matchers.h"
#include "../utils/OpenMP.h"
#include "../utils/OptionsUtils.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/ADT/SmallSet.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OpenMPClause.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtOpenMP.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/ASTMatchers/ASTMatchersMacros.h>
#include <clang/Analysis/Analyses/ExprMutationAnalyzer.h>
#include <clang/Basic/Builtins.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <cstddef>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SetOperations.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Frontend/OpenMP/OMPConstants.h>
#include <llvm/Support/Casting.h>
#include <utility>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::MapAnyOfMatcher<
    OMPCriticalDirective, OMPAtomicDirective, OMPOrderedDirective,
    OMPSingleDirective>
    ompProtectedAccessDirective;

const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPTaskDirective>
    ompTaskDirective;
// NOLINTEND(readability-identifier-naming)

AST_MATCHER(CallExpr, isCallingAtomicBuiltin) {
  switch (Node.getBuiltinCallee()) {
#define BUILTIN(ID, TYPE, ATTRS)
#define ATOMIC_BUILTIN(ID, TYPE, ATTRS)                                        \
  case Builtin::BI##ID:                                                        \
    return true;
#include "clang/Basic/Builtins.inc"
  default:
    return false;
  }
}

AST_MATCHER(OMPExecutableDirective, isOMPParallelDirective) {
  return isOpenMPParallelDirective(Node.getDirectiveKind());
}

AST_MATCHER(OMPExecutableDirective, isOMPTargetDirective) {
  return isOpenMPTargetExecutionDirective(Node.getDirectiveKind());
}

class DependentVariableState {
public:
  void add(const OMPExecutableDirective *const Directive) {
    DependentVarsStack.push_back(getDependVariables(Directive));
    AllTimeDependentVars.insert(DependentVarsStack.back().begin(),
                                DependentVarsStack.back().end());
  }

  void pop() { DependentVarsStack.pop_back(); }

  bool isDependent(const ValueDecl *Var) const {
    return !DependentVarsStack.empty() &&
           llvm::is_contained(DependentVarsStack.back(), Var);
  }

  bool wasAtSomePointDependent(const ValueDecl *Var) const {
    return llvm::find(AllTimeDependentVars, Var) != AllTimeDependentVars.end();
  }

private:
  llvm::SmallPtrSet<const ValueDecl *, 4> AllTimeDependentVars;
  llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>> DependentVarsStack;
};

class DirectiveState {
public:
  void add(const OMPExecutableDirective *Directive) {
    DirectiveStack.push_back(Directive);
    AncestorContext.push_back(Directive->getDirectiveKind());
  }
  void pop() {
    DirectiveStack.pop_back();
    AncestorContext.pop_back();
  }
  llvm::SmallVector<const OMPExecutableDirective *> DirectiveStack;
  llvm::SmallVector<OpenMPDirectiveKind, 8> AncestorContext;
};

class SharedAndPrivateState {
public:
  void add(const OMPExecutableDirective *const Directive) {
    const auto Shared = getSharedVariables(Directive);
    for (const ValueDecl *SharedVar : Shared) {
      auto *const Iter = llvm::find_if(CurrentSharedVars,
                                       [SharedVar](const auto &VarAndCount) {
                                         return VarAndCount.first == SharedVar;
                                       });
      if (Iter != CurrentSharedVars.end())
        ++Iter->second;
      else {
        CurrentSharedVars.emplace_back(SharedVar, 1U);
        AllTimeSharedVars.insert(SharedVar);
      }
    }

    llvm::SmallVector<std::pair<const ValueDecl *, size_t>> PrivatizedVariables;

    for (const ValueDecl *PrivateVar : getPrivatizedVariables(Directive)) {
      auto *const Iter = llvm::find_if(CurrentSharedVars,
                                       [PrivateVar](const auto &VarAndCount) {
                                         return VarAndCount.first == PrivateVar;
                                       });
      if (Iter != CurrentSharedVars.end()) {
        PrivatizedVariables.push_back(*Iter);
        CurrentSharedVars.erase(Iter);
        AllTimeSharedVars.erase(Iter->first);
      }
    }

    SharedVarsStack.push_back(Shared);
    PrivatizedVarsStack.push_back(PrivatizedVariables);
  }

  void pop() {
    CurrentSharedVars.append(PrivatizedVarsStack.back());
    PrivatizedVarsStack.pop_back();
    const llvm::SmallPtrSet<const ValueDecl *, 4> &LatestSharedScopeChange =
        SharedVarsStack.back();
    for (const ValueDecl *const SharedVar : LatestSharedScopeChange) {
      auto *const Iter = llvm::find_if(CurrentSharedVars,
                                       [SharedVar](const auto &VarAndCount) {
                                         return VarAndCount.first == SharedVar;
                                       });
      if (Iter != CurrentSharedVars.end()) {
        --Iter->second;
        if (Iter->second == 0)
          CurrentSharedVars.erase(Iter);
      }
    }
    SharedVarsStack.pop_back();
  }

  bool isShared(const ValueDecl *Var) const {
    return llvm::find_if(CurrentSharedVars,
                         [Var](const auto &SharedVarWithCount) {
                           return SharedVarWithCount.first == Var;
                         }) != CurrentSharedVars.end();
  }

  bool wasAtSomePointShared(const ValueDecl *Var) const {
    return llvm::find(AllTimeSharedVars, Var) != AllTimeSharedVars.end();
  }

  llvm::SmallVector<std::pair<const ValueDecl *, size_t>> CurrentSharedVars;
  llvm::SmallPtrSet<const ValueDecl *, 4> AllTimeSharedVars;
  llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>> SharedVarsStack;
  llvm::SmallVector<llvm::SmallVector<std::pair<const ValueDecl *, size_t>>>
      PrivatizedVarsStack;
};

class ReductionState {
public:
  void add(const OMPExecutableDirective *Directive) {
    ReductionVarsStack.push_back({});
    llvm::set_union(ReductionVarsStack.back(),
                    getCaptureDeclsOf<OMPReductionClause>(Directive));
    llvm::set_union(ReductionVarsStack.back(),
                    getCaptureDeclsOf<OMPInReductionClause>(Directive));
    llvm::set_union(ReductionVarsStack.back(),
                    getCaptureDeclsOf<OMPTaskReductionClause>(Directive));

    llvm::set_union(CurrentReductionVariables, ReductionVarsStack.back());
  }

  void pop() {
    llvm::set_difference(CurrentReductionVariables, ReductionVarsStack.back());
    ReductionVarsStack.pop_back();
  }

  bool isReductionVar(const ValueDecl *const Var) {
    return CurrentReductionVariables.contains(Var);
  }

private:
  llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>> ReductionVarsStack;
  llvm::SmallPtrSet<const ValueDecl *, 4> CurrentReductionVariables;
};

class VariableState {
public:
  void add(const OMPExecutableDirective *Directive) {
    SharedAndPrivateVars.add(Directive);
    Directives.add(Directive);
    DependentVars.add(Directive);
    Reductions.add(Directive);
  }

  void pop() {
    SharedAndPrivateVars.pop();
    Directives.pop();
    DependentVars.pop();
    Reductions.pop();
  }

  DependentVariableState DependentVars;
  SharedAndPrivateState SharedAndPrivateVars;
  DirectiveState Directives;
  ReductionState Reductions;
};

class Visitor : public RecursiveASTVisitor<Visitor> {
public:
  Visitor(ASTContext &Ctx, llvm::ArrayRef<llvm::StringRef> ThreadSafeTypes,
          llvm::ArrayRef<llvm::StringRef> ThreadSafeFunctions)
      : Ctx(Ctx), ThreadSafeTypes(ThreadSafeTypes),
        ThreadSafeFunctions(ThreadSafeFunctions) {}

  using Base = RecursiveASTVisitor<Visitor>;
  struct AnalysisResult {
    struct Result {
      const Stmt *S;
      llvm::SmallVector<OpenMPDirectiveKind, 8> AncestorContext;
      bool IsDependent = false;
      bool WasEverDependent = false;

      template <typename... OMPDirectiveKinds>
      bool isInContextOf(const OMPDirectiveKinds... Expected) const {
        return llvm::any_of(
            AncestorContext, [Expected...](const OpenMPDirectiveKind Kind) {
              return (isOpenMPDirectiveKind(Kind, Expected) || ...);
            });
      }

      friend bool operator<(const Result &Lhs, const Result &Rhs) {
        return Lhs.S < Rhs.S;
      }
      friend bool operator==(const Result &Lhs, const Result &Rhs) {
        return Lhs.S == Rhs.S;
      }
    };

    llvm::SmallSet<Result, 4> Mutations;
    llvm::SmallSet<Result, 4> UnprotectedAcesses;
    llvm::SmallSet<Result, 4> UnprotectedDependentAcesses;
  };

  bool TraverseOMPExecutableDirective(OMPExecutableDirective *Directive) {
    if (Directive->isStandaloneDirective()) {
      if (const auto *const Barrier =
              llvm::dyn_cast<OMPBarrierDirective>(Directive))
        saveAnalysisAndStartNewEpoch();
      else
        startNewEpochIfEncountered<OMPTaskwaitDirective>(Directive);

      return true;
    }

    markCapturesAsMutations<OMPReductionClause>(Directive);
    markCapturesAsMutations<OMPTaskReductionClause>(Directive);
    markCapturesAsMutations<OMPInReductionClause>(Directive);

    State.add(Directive);
    Stmt *Statement = Directive->getStructuredBlock();
    Base::TraverseStmt(Statement);
    State.pop();

    if (hasBarrier(Directive, Ctx)) {
      if (!(startNewEpochIfEncountered<OMPTaskDirective>(Directive) ||
            startNewEpochIfEncountered<OMPTaskgroupDirective>(Directive)))
        saveAnalysisAndStartNewEpoch();
    }

    return true;
  }

  bool TraverseCapturedStmt(CapturedStmt *S) { return true; }

  bool TraverseDeclRefExpr(DeclRefExpr *DRef) {
    const ValueDecl *Var = DRef->getDecl();
    const bool IsShared = State.SharedAndPrivateVars.isShared(Var);
    const bool WasAtSomePointDependent =
        State.DependentVars.wasAtSomePointDependent(Var);
    const bool WasAtSomePointShared =
        State.SharedAndPrivateVars.wasAtSomePointShared(Var);
    if (!IsShared && !WasAtSomePointDependent && !WasAtSomePointShared)
      return true;

    const bool IsReductionVariable = State.Reductions.isReductionVar(Var);
    bool IsProtected = IsReductionVariable;
    if (!IsProtected) {
      // FIXME: can use traversal to know if there is an ancestor
      const auto MatchResult =
          match(declRefExpr(hasAncestor(ompExecutableDirective(
                    anyOf(ompProtectedAccessDirective().bind("protected"),
                          ompTaskDirective())))),
                *DRef, Ctx);
      IsProtected =
          !MatchResult.empty() &&
          MatchResult[0].getNodeAs<OMPExecutableDirective>("protected");
    }

    const bool IsDependent = State.DependentVars.isDependent(Var);

    if (!IsProtected) {
      if (IsDependent)
        ContextResults[Var].UnprotectedDependentAcesses.insert(
            AnalysisResult::Result{DRef, State.Directives.AncestorContext,
                                   IsDependent, WasAtSomePointDependent});
      else
        ContextResults[Var].UnprotectedAcesses.insert(
            AnalysisResult::Result{DRef, State.Directives.AncestorContext,
                                   IsDependent, WasAtSomePointDependent});
    }

    if (isMutating(DRef))
      ContextResults[Var].Mutations.insert(
          AnalysisResult::Result{DRef, State.Directives.AncestorContext,
                                 IsDependent, WasAtSomePointDependent});

    return true;
  }

  bool TraverseFunctionDecl(FunctionDecl *const) { return true; }

  bool TraverseCallExpr(CallExpr *const CE) {
    for (Expr *const Arg : CE->arguments())
      Base::TraverseStmt(Arg);
    const auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::is_contained(CallStack, FDecl))
      return true;
    CallStack.push_back(FDecl);
    if (FDecl->hasBody())
      TraverseStmt(FDecl->getBody());
    CallStack.pop_back();
    return true;
  }

  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *const CE) {
    for (Expr *const Arg : CE->arguments())
      Base::TraverseStmt(Arg);
    const auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::is_contained(CallStack, FDecl))
      return true;
    CallStack.push_back(FDecl);
    if (FDecl->hasBody())
      TraverseStmt(FDecl->getBody());
    CallStack.pop_back();
    return true;
  }

  bool isMutating(const DeclRefExpr *DRef) const {
    if (State.Directives.DirectiveStack.empty())
      return false;
    const ValueDecl *Dec = DRef->getDecl();

    const auto ThreadSafeType = qualType(anyOf(
        atomicType(),
        qualType(matchers::matchesAnyListedTypeName(ThreadSafeTypes)),
        hasCanonicalType(matchers::matchesAnyListedTypeName(ThreadSafeTypes))));

    const auto CollidingIndex =
        expr(anyOf(declRefExpr(to(varDecl(
                       anyOf(isConstexpr(), hasType(isConstQualified()))))),
                   constantExpr(), integerLiteral()));

    const auto IsCastToRValueOrConst = traverse(
        TK_AsIs,
        expr(hasParent(implicitCastExpr(
            anyOf(hasCastKind(CastKind::CK_LValueToRValue),
                  hasImplicitDestinationType(qualType(isConstQualified())))))));

    const auto AtomicIntrinsicCall = callExpr(isCallingAtomicBuiltin());

    const auto Var = declRefExpr(to(
        // `Dec` or a binding if `Dec` is a decomposition.
        anyOf(equalsNode(Dec), bindingDecl(forDecomposition(equalsNode(Dec))))
        //
        ));

    const bool ShouldBeDiagnosed =
        !match(traverse(
                   TK_IgnoreUnlessSpelledInSource,
                   declRefExpr(
                       Var, unless(hasType(ThreadSafeType)),
                       unless(hasParent(expr(anyOf(
                           arraySubscriptExpr(
                               unless(allOf(hasIndex(CollidingIndex),
                                            unless(hasType(ThreadSafeType))))),
                           cxxOperatorCallExpr(hasOverloadedOperatorName("[]"),
                                               hasArgument(0, Var)),
                           unaryOperator(hasOperatorName("&"),
                                         hasUnaryOperand(Var),
                                         hasParent(AtomicIntrinsicCall)),
                           callExpr(
                               callee(namedDecl(matchers::matchesAnyListedName(
                                   ThreadSafeFunctions)))),
                           IsCastToRValueOrConst, AtomicIntrinsicCall)))))
                       .bind("dref")),
               *DRef, Ctx)
             .empty();

    if (!ShouldBeDiagnosed)
      return false;

    ExprMutationAnalyzer Analyzer(
        *State.Directives.DirectiveStack.back()->getStructuredBlock(), Ctx);

    return Analyzer.isMutated(DRef);
  }

  template <typename Clause>
  void markCapturesAsMutations(const OMPExecutableDirective *const Directive) {
    for (const ValueDecl *const Val : getCaptureDeclsOf<Clause>(Directive))
      ContextResults[Val].Mutations.insert(AnalysisResult::Result{
          Directive, State.Directives.AncestorContext,
          State.DependentVars.isDependent(Val),
          State.DependentVars.wasAtSomePointDependent(Val)});
  }

  template <typename DirectiveType>
  bool
  startNewEpochIfEncountered(const OMPExecutableDirective *const Directive) {
    const auto *const CastDirective = llvm::dyn_cast<DirectiveType>(Directive);
    if (!CastDirective)
      return false;

    const auto Clauses = CastDirective->clauses();
    if (Clauses.empty()) {
      saveAnalysisAndStartNewEpoch();
      return true;
    }

    bool Result = false;
    for (const OMPClause *const Clause : Clauses)
      if (const auto *const Depend = llvm::dyn_cast<OMPDependClause>(Clause))
        for (const auto *const VarExpr : Depend->getVarRefs())
          if (const auto *const DRef = llvm::dyn_cast<DeclRefExpr>(VarExpr)) {
            saveAnalysisAndStartNewEpoch(DRef->getDecl());
            Result = true;
          }

    return Result;
  }

  llvm::SmallVector<
      std::pair<const clang::ValueDecl *, Visitor::AnalysisResult>, 4>
  takeResults() {
    Results.append(ContextResults.takeVector());
    return Results;
  }

private:
  void saveAnalysisAndStartNewEpoch() {
    Results.append(ContextResults.takeVector());
    ContextResults = llvm::MapVector<const ValueDecl *, AnalysisResult>{};
  }

  void saveAnalysisAndStartNewEpoch(const ValueDecl *const Val) {
    auto ResultsForValIter = ContextResults.find(Val);
    if (ResultsForValIter == ContextResults.end())
      return;
    Results.push_back(*ResultsForValIter);
    ResultsForValIter->second.Mutations.clear();
    ResultsForValIter->second.UnprotectedAcesses.clear();
    ResultsForValIter->second.UnprotectedDependentAcesses.clear();
  }

  llvm::MapVector<const ValueDecl *, AnalysisResult> ContextResults;

  llvm::SmallVector<
      std::pair<const clang::ValueDecl *, Visitor::AnalysisResult>, 4>
      Results;

  llvm::SmallVector<const Decl *> CallStack;

  VariableState State;
  ASTContext &Ctx;
  const llvm::ArrayRef<llvm::StringRef> ThreadSafeTypes;
  const llvm::ArrayRef<llvm::StringRef> ThreadSafeFunctions;
};

const auto DefaultThreadSafeTypes = "std::atomic.*;std::atomic_ref.*";
const auto DefaultThreadSafeFunctions = "";
} // namespace

void UnprotectedSharedVariableAccessCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      ompExecutableDirective(
          anyOf(isOMPParallelDirective(), isOMPTargetDirective()),
          unless(isStandaloneDirective()),
          unless(hasAncestor(ompExecutableDirective())))
          .bind("directive"),
      this);
}

void UnprotectedSharedVariableAccessCheck::check(
    const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;

  const auto *const Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");

  Visitor V(Ctx, ThreadSafeTypes, ThreadSafeFunctions);
  V.TraverseStmt(const_cast<OMPExecutableDirective *>(Directive));
  for (const auto &[SharedVar, Res] : V.takeResults()) {
    const auto &[Mutations, UnprotectedAcesses, UnprotectedDependentAccesses] =
        Res;

    if (Mutations.empty())
      continue;

    const bool AllMutationsAreInMaster =
        llvm::all_of(Mutations, [](const Visitor::AnalysisResult::Result &Res) {
          return Res.isInContextOf(OpenMPDirectiveKind::OMPD_master);
        });

    for (const auto &UnprotectedAccess : UnprotectedAcesses) {
      if (AllMutationsAreInMaster &&
          UnprotectedAccess.isInContextOf(OpenMPDirectiveKind::OMPD_master))
        continue;

      diag(UnprotectedAccess.S->getBeginLoc(),
           "do not access shared variable %0 of type %1 without "
           "synchronization%select{|; specify synchronization on the task with "
           "`depend`}2")
          << UnprotectedAccess.S->getSourceRange() << SharedVar
          << SharedVar->getType()
          << UnprotectedAccess.isInContextOf(OpenMPDirectiveKind::OMPD_task);
      for (const auto &[Mutation, MutationContext, MutationIsDependent,
                        MutationWasEverDependent] : Mutations)
        diag(Mutation->getBeginLoc(), "%0 was mutated here",
             DiagnosticIDs::Level::Note)
            << Mutation->getSourceRange() << SharedVar;
    }

    const bool AllMutationsAreDependent =
        llvm::all_of(Mutations, [](const Visitor::AnalysisResult::Result &R) {
          return R.IsDependent;
        });

    const bool AllMutationsAreInTasks =
        llvm::all_of(Mutations, [](const Visitor::AnalysisResult::Result &R) {
          return R.isInContextOf(OpenMPDirectiveKind::OMPD_task);
        });

    if (!UnprotectedAcesses.empty() || AllMutationsAreInTasks)
      continue;

    for (const auto &UnprotectedAccess : UnprotectedDependentAccesses) {
      const bool IsInSingleOrSections = UnprotectedAccess.isInContextOf(
          OpenMPDirectiveKind::OMPD_sections, OpenMPDirectiveKind::OMPD_single);

      if (UnprotectedAcesses.empty() &&
          (IsInSingleOrSections || AllMutationsAreDependent))
        continue;
      diag(UnprotectedAccess.S->getBeginLoc(),
           "do not access shared variable %0 of type %1 without "
           "synchronization")
          << UnprotectedAccess.S->getSourceRange() << SharedVar
          << SharedVar->getType();
      for (const auto &[Mutation, MutationContext, MutationIsDependent,
                        MutationWasEverDependent] : Mutations)
        diag(Mutation->getBeginLoc(), "%0 was mutated here",
             DiagnosticIDs::Level::Note)
            << Mutation->getSourceRange() << SharedVar;
    }
  }
}

void UnprotectedSharedVariableAccessCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "ThreadSafeTypes",
                utils::options::serializeStringList(ThreadSafeTypes));
  Options.store(Opts, "ThreadSafeFunctions",
                utils::options::serializeStringList(ThreadSafeFunctions));
}

UnprotectedSharedVariableAccessCheck::UnprotectedSharedVariableAccessCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      ThreadSafeTypes(utils::options::parseStringList(
          Options.get("ThreadSafeTypes", DefaultThreadSafeTypes))),
      ThreadSafeFunctions(utils::options::parseStringList(
          Options.get("ThreadSafeFunctions", DefaultThreadSafeFunctions))) {}
} // namespace clang::tidy::openmp
