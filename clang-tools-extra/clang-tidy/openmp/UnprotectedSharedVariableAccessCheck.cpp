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

bool isOpenMPDirectiveKind(const OpenMPDirectiveKind DKind,
                           const OpenMPDirectiveKind Expected) {
  return DKind == Expected ||
         llvm::is_contained(getLeafConstructs(DKind), Expected);
}

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

  class SharedVariableState {
  public:
    void add(const OMPExecutableDirective *Directive) {
      const openmp::SharedAndPrivateVariables SharedAndPrivateVars =
          openmp::getSharedAndPrivateVariable(Directive);
      for (const ValueDecl *SharedVar : SharedAndPrivateVars.Shared) {
        auto *const Iter = llvm::find_if(
            CurrentSharedVariables, [SharedVar](const auto &VarAndCount) {
              return VarAndCount.first == SharedVar;
            });
        if (Iter != CurrentSharedVariables.end())
          ++Iter->second;
        else
          CurrentSharedVariables.emplace_back(SharedVar, 1U);
      }

      llvm::SmallVector<std::pair<const ValueDecl *, size_t>>
          PrivatizedVariables;

      for (const ValueDecl *PrivateVar : SharedAndPrivateVars.Private) {
        auto *const Iter = llvm::find_if(
            CurrentSharedVariables, [PrivateVar](const auto &VarAndCount) {
              return VarAndCount.first == PrivateVar;
            });
        if (Iter != CurrentSharedVariables.end()) {
          PrivatizedVariables.push_back(*Iter);
          CurrentSharedVariables.erase(Iter);
        }
      }

      SharedVarsStack.push_back(SharedAndPrivateVars.Shared);
      PrivatizedVarsStack.push_back(PrivatizedVariables);
      DependentVarsStack.push_back(SharedAndPrivateVars.Dependent);
      CurrentDependentVariables = DependentVarsStack.back();
      AllTimeDependentVariables.insert(DependentVarsStack.back().begin(),
                                       DependentVarsStack.back().end());
      DirectiveStack.push_back(Directive);
      AncestorContext.push_back(Directive->getDirectiveKind());
    }

    void pop() {
      CurrentSharedVariables.append(PrivatizedVarsStack.back());
      PrivatizedVarsStack.pop_back();
      const llvm::SmallPtrSet<const ValueDecl *, 4> &LatestSharedScopeChange =
          SharedVarsStack.back();
      for (const ValueDecl *const SharedVar : LatestSharedScopeChange) {
        auto *const Iter = llvm::find_if(
            CurrentSharedVariables, [SharedVar](const auto &VarAndCount) {
              return VarAndCount.first == SharedVar;
            });
        if (Iter != CurrentSharedVariables.end()) {
          --Iter->second;
          if (Iter->second == 0)
            CurrentSharedVariables.erase(Iter);
        }
      }
      SharedVarsStack.pop_back();
      DirectiveStack.pop_back();
      AncestorContext.pop_back();
      DependentVarsStack.pop_back();

      if (!DependentVarsStack.empty())
        CurrentDependentVariables = DependentVarsStack.back();
      else
        CurrentDependentVariables = {};
    }

    bool isShared(const ValueDecl *Var) const {
      return llvm::find_if(CurrentSharedVariables,
                           [Var](const auto &SharedVarWithCount) {
                             return SharedVarWithCount.first == Var;
                           }) != CurrentSharedVariables.end();
    }

    bool isDependent(const ValueDecl *Var) const {
      return llvm::is_contained(CurrentDependentVariables, Var);
    }

    bool wasAtSomePointDependent(const ValueDecl *Var) const {
      return llvm::find(AllTimeDependentVariables, Var) !=
             AllTimeDependentVariables.end();
    }

    // private:
    llvm::SmallVector<std::pair<const ValueDecl *, size_t>>
        CurrentSharedVariables;
    llvm::SmallPtrSet<const ValueDecl *, 4> CurrentDependentVariables;
    llvm::SmallPtrSet<const ValueDecl *, 4> AllTimeDependentVariables;
    llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>> SharedVarsStack;
    llvm::SmallVector<llvm::SmallVector<std::pair<const ValueDecl *, size_t>>>
        PrivatizedVarsStack;
    llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>>
        DependentVarsStack;
    llvm::SmallVector<const OMPExecutableDirective *> DirectiveStack;
    llvm::SmallVector<OpenMPDirectiveKind, 8> AncestorContext;
  };

  bool TraverseOMPExecutableDirective(OMPExecutableDirective *Directive) {
    if (Directive->isStandaloneDirective()) {
      if (const auto *const Barrier =
              llvm::dyn_cast<OMPBarrierDirective>(Directive))
        saveAnalysisAndStartNewEpoch();

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

    startNewEpochIfEncountered<OMPTaskgroupDirective>(Directive);
    return true;
  }

  bool TraverseCapturedStmt(CapturedStmt *S) { return true; }

  bool TraverseDeclRefExpr(DeclRefExpr *DRef) {
    const ValueDecl *Var = DRef->getDecl();
    const bool IsShared = State.isShared(Var);
    const bool WasAtSomePointDependent = State.wasAtSomePointDependent(Var);
    if (!IsShared && !WasAtSomePointDependent)
      return true;

    // FIXME: can use traversal to know if there is an ancestor
    const auto MatchResult =
        match(declRefExpr(hasAncestor(ompExecutableDirective(
                  anyOf(ompProtectedAccessDirective().bind("protected"),
                        ompTaskDirective())))),
              *DRef, Ctx);
    const bool IsProtected =
        !MatchResult.empty() &&
        MatchResult[0].getNodeAs<OMPExecutableDirective>("protected");

    const bool IsDependent = State.isDependent(Var);

    if (!IsProtected) {
      if (IsDependent)
        ContextResults[Var].UnprotectedDependentAcesses.insert(
            AnalysisResult::Result{DRef, State.AncestorContext, IsDependent,
                                   WasAtSomePointDependent});
      else
        ContextResults[Var].UnprotectedAcesses.insert(AnalysisResult::Result{
            DRef, State.AncestorContext, IsDependent, WasAtSomePointDependent});
    }

    if (isMutating(DRef))
      ContextResults[Var].Mutations.insert(AnalysisResult::Result{
          DRef, State.AncestorContext, IsDependent, WasAtSomePointDependent});

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
    if (State.DirectiveStack.empty())
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
        *State.DirectiveStack.back()->getStructuredBlock(), Ctx);

    return Analyzer.isMutated(DRef);
  }

  template <typename Clause>
  void markCapturesAsMutations(const OMPExecutableDirective *const Directive) {
    for (const ValueDecl *const Val : getCaptureDeclsOf<Clause>(Directive))
      ContextResults[Val].Mutations.insert(AnalysisResult::Result{
          Directive, State.AncestorContext, State.isDependent(Val),
          State.wasAtSomePointDependent(Val)});
  }

  template <typename DirectiveType>
  void
  startNewEpochIfEncountered(const OMPExecutableDirective *const Directive) {
    if (const auto *const Taskgroup =
            llvm::dyn_cast<DirectiveType>(Directive)) {
      const auto Clauses = Taskgroup->clauses();
      if (Clauses.empty()) {
        saveAnalysisAndStartNewEpoch();
        return;
      }
      for (const OMPClause *const Clause : Clauses)
        if (const auto *const Depend = llvm::dyn_cast<OMPDependClause>(Clause))
          for (const auto *const VarExpr : Depend->getVarRefs())
            if (const auto *const DRef = llvm::dyn_cast<DeclRefExpr>(VarExpr))
              saveAnalysisAndStartNewEpoch(DRef->getDecl());
    }
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

  SharedVariableState State;
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

    for (const auto &UnprotectedAccess : UnprotectedAcesses) {
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
