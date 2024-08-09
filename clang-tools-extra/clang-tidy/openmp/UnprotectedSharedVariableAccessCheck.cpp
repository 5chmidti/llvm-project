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
bool isOMPLockType(const QualType &QType) {
  if (QType.isNull())
    return false;

  const std::string UnqualifiedTypeAsString =
      QualType(QType->getUnqualifiedDesugaredType(), 0).getAsString();

  return UnqualifiedTypeAsString == "struct omp_lock_t";
}

class DependentVariableState {
public:
  void add(const OMPExecutableDirective *const Directive) {
    DependentVarsStack.push_back(getDependVariables(Directive));
    CurrentDependentVars = DependentVarsStack.back();
    AllTimeDependentVars.insert(DependentVarsStack.back().begin(),
                                DependentVarsStack.back().end());
  }

  void pop() {
    DependentVarsStack.pop_back();

    if (!DependentVarsStack.empty())
      CurrentDependentVars = DependentVarsStack.back();
    else
      CurrentDependentVars = {};
  }

  bool isDependent(const ValueDecl *Var) const {
    return CurrentDependentVars.contains(Var);
  }

  bool wasAtSomePointDependent(const ValueDecl *Var) const {
    return AllTimeDependentVars.contains(Var);
  }

private:
  llvm::SmallPtrSet<const ValueDecl *, 4> AllTimeDependentVars;
  llvm::SmallPtrSet<const ValueDecl *, 4> CurrentDependentVars;
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

  bool isInTeamsDirective() const {
    return llvm::any_of(AncestorContext, [](const OpenMPDirectiveKind Kind) {
      return isOpenMPDirectiveKind(Kind, OpenMPDirectiveKind::OMPD_teams);
    });
  }

  template <typename... Directives> bool isInDirective() {
    return llvm::any_of(DirectiveStack, [](const OMPExecutableDirective *D) {
      return (llvm::isa<Directives>(D) || ...);
    });
  }

  bool isInDirective(const OpenMPDirectiveKind Kind) {
    return llvm::any_of(AncestorContext, [Kind](const OpenMPDirectiveKind D) {
      return D == Kind;
    });
  }

  llvm::SmallVector<const OMPExecutableDirective *> DirectiveStack;
  llvm::SmallVector<OpenMPDirectiveKind, 8> AncestorContext;
};

class SharedAndPrivateState {
public:
  enum class State {
    Private,
    Shared,
    InnerLocal,
    Parameter,
  };

  explicit SharedAndPrivateState(const ASTContext &Ctx) : Ctx{Ctx} {}

  void add(const OMPExecutableDirective *const Directive) {
    const auto Shared = getSharedVariables(Directive, Ctx);
    const auto Private = getPrivatizedVariables(Directive, Ctx);

    ChangedVars.push_back({});

    if (Shared.empty() && Private.empty())
      return;

    for (const ValueDecl *SharedVar : Shared) {
      Vars[SharedVar].push(State::Shared);
      ChangedVars.back().insert(SharedVar);
    }

    for (const ValueDecl *PrivateVar : Private) {
      Vars[PrivateVar].push(State::Private);
      ChangedVars.back().insert(PrivateVar);
    }
  }

  void addGlobal(const VarDecl *const Global) {
    Vars[Global].push(State::Shared);
  }

  void addInnerLocal(const VarDecl *const InnerLocal) {
    Vars[InnerLocal].push(State::InnerLocal);
  }

  void addParm(const VarDecl *const Parm) { Vars[Parm].push(State::Parameter); }

  void pop() {
    if (ChangedVars.empty())
      return;

    for (const ValueDecl *const Changed : ChangedVars.back())
      Vars[Changed].pop();

    ChangedVars.pop_back();
  }

  bool is(const ValueDecl *Var, const State S) const {
    const auto Iter = Vars.find(Var);
    if (Iter == Vars.end())
      return false;

    return Iter->second.is(S);
  }

  bool was(const ValueDecl *Var, const State S) const {
    const auto Iter = Vars.find(Var);
    if (Iter == Vars.end())
      return false;

    return Iter->second.was(S);
  }

  bool isUnknown(const ValueDecl *Var) const {
    const auto Iter = Vars.find(Var);
    if (Iter == Vars.end())
      return true;

    return Iter->second.isUnknown();
  }

private:
  class StateInfo {
  public:
    void push(const State S) {
      CurrentState.push_back(S);
      AllTime.insert(S);
    }
    void pop() { CurrentState.pop_back(); }

    bool is(const State S) const {
      return !CurrentState.empty() && CurrentState.back() == S;
    }
    bool was(const State S) const { return AllTime.contains(S); }
    bool isUnknown() const { return CurrentState.empty(); }

  private:
    llvm::SmallVector<State> CurrentState;
    llvm::SmallSet<State, 3> AllTime;
  };

  const ASTContext &Ctx;

  std::map<const ValueDecl *, StateInfo> Vars;
  std::vector<llvm::SmallPtrSet<const ValueDecl *, 4>> ChangedVars;
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
    CurrentReductionVariables = llvm::set_difference(CurrentReductionVariables,
                                                     ReductionVarsStack.back());
    ReductionVarsStack.pop_back();
  }

  bool isReductionVar(const ValueDecl *const Var) const {
    return CurrentReductionVariables.contains(Var);
  }

private:
  llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>> ReductionVarsStack;
  llvm::SmallPtrSet<const ValueDecl *, 4> CurrentReductionVariables;
};

class TargetState {
public:
  void add(const OMPExecutableDirective *Directive) {
    MappedStack.push_back({});
    llvm::set_union(MappedStack.back(),
                    getMappedDeclsOf<OMPMapClause>(Directive));
    llvm::set_union(MappedStack.back(),
                    getMappedDeclsOf<OMPDefaultmapClause>(Directive));
  }

  void pop() { MappedStack.pop_back(); }

  bool isMapped(const ValueDecl *const Var) const {
    return llvm::any_of(
        MappedStack, [Var](const llvm::SmallPtrSet<const ValueDecl *, 4> &Set) {
          return Set.contains(Var);
        });
  }

private:
  llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>> MappedStack;
};

class VariableState {
public:
  explicit VariableState(const ASTContext &Ctx) : SharedAndPrivateVars(Ctx) {}

  void add(const OMPExecutableDirective *Directive) {
    SharedAndPrivateVars.add(Directive);
    Directives.add(Directive);
    DependentVars.add(Directive);
    Reductions.add(Directive);
    Target.add(Directive);
  }

  void pop() {
    SharedAndPrivateVars.pop();
    Directives.pop();
    DependentVars.pop();
    Reductions.pop();
    Target.pop();
  }

  DependentVariableState DependentVars;
  SharedAndPrivateState SharedAndPrivateVars;
  DirectiveState Directives;
  ReductionState Reductions;
  TargetState Target;
};

class Visitor : public RecursiveASTVisitor<Visitor> {
public:
  Visitor(ASTContext &Ctx, llvm::ArrayRef<llvm::StringRef> ThreadSafeTypes,
          llvm::ArrayRef<llvm::StringRef> ThreadSafeFunctions)
      : State(Ctx), Ctx(Ctx), ThreadSafeTypes(ThreadSafeTypes),
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
    const bool IsParallelDirective = isOpenMPDirectiveKind(
        Directive->getDirectiveKind(), llvm::omp::Directive::OMPD_parallel,
        llvm::omp::Directive::OMPD_teams);
    if (IsParallelDirective)
      ++ParallelContextDepth;

    if (ParallelContextDepth == 0)
      return true;

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

    if (IsParallelDirective)
      --ParallelContextDepth;

    return true;
  }

  // Have to force traversal to only go through the custom
  // 'TraverseOMPExecutableDirective'. Otherwise, the
  // 'Base::TraverseOMPCriticalDirective' would call
  // 'getDerived().TraverseOMPExecutableDirective' before traversing
  // it's children, which would cause visitation of the statements
  // inside the directive to be traversed twice.
  bool TraverseOMPCriticalDirective(OMPCriticalDirective *D) {
    return TraverseOMPExecutableDirective(D);
  }
  bool TraverseOMPAtomicDirective(OMPAtomicDirective *D) {
    return TraverseOMPExecutableDirective(D);
  }
  bool TraverseOMPSingleDirective(OMPSingleDirective *D) {
    return TraverseOMPExecutableDirective(D);
  }

  bool TraverseVarDecl(VarDecl *V) {
    if (V->hasGlobalStorage())
      State.SharedAndPrivateVars.addGlobal(V);

    if (ParallelContextDepth != 0)
      State.SharedAndPrivateVars.addInnerLocal(V);

    return Base::TraverseVarDecl(V);
  }

  bool TraverseParmVarDecl(ParmVarDecl *Parm) {
    State.SharedAndPrivateVars.addParm(Parm);

    return Base::TraverseParmVarDecl(Parm);
  }

  bool TraverseCapturedStmt(CapturedStmt *S) { return true; }
  bool TraverseCapturedDecl(CapturedDecl *D) { return true; }

  bool TraverseDeclRefExpr(DeclRefExpr *DRef) {
    if (ParallelContextDepth == 0)
      return true;

    // FIXME: store the analysis of Vars in a cache
    const ValueDecl *Var = DRef->getDecl();

    if (llvm::isa<EnumConstantDecl, FunctionDecl>(Var))
      return true;

    const auto *const Variable = llvm::dyn_cast<VarDecl>(Var);
    const bool IsThreadLocal = Variable && Variable->getStorageDuration() ==
                                               StorageDuration::SD_Thread;
    if (isOMPLockType(Var->getType()) ||
        Var->hasAttr<OMPThreadPrivateDeclAttr>() || IsThreadLocal)
      return true;

    const bool IsShared = State.SharedAndPrivateVars.is(
        Var, SharedAndPrivateState::State::Shared);
    const bool WasAtSomePointDependent =
        State.DependentVars.wasAtSomePointDependent(Var);
    const bool WasAtSomePointShared = State.SharedAndPrivateVars.was(
        Var, SharedAndPrivateState::State::Shared);
    const bool IsPrivate = State.SharedAndPrivateVars.is(
        Var, SharedAndPrivateState::State::Private);
    const bool IsUnknown = State.SharedAndPrivateVars.isUnknown(Var);
    const bool Global = Variable && Variable->hasGlobalStorage();
    const bool IsMapped = State.Target.isMapped(Var);
    if (!IsUnknown &&
        ((!IsShared && !WasAtSomePointDependent && !WasAtSomePointShared &&
          !Global && !IsMapped) ||
         (!WasAtSomePointShared &&
          !WasAtSomePointDependent) || // FIXME: currently dependent?
         IsPrivate
         )) {
      return true;
    }

    const bool IsReductionVariable = State.Reductions.isReductionVar(Var);
    bool IsProtected =
        LockedRegionCount > 0 || IsReductionVariable ||
        State.Directives
            .isInDirective<OMPCriticalDirective, OMPAtomicDirective>();
    if (!IsProtected) {
      const auto GetThreadNum =
          callExpr(callee(functionDecl(hasName("::omp_get_thread_num"))));
      const auto ThreadNum =
          expr(anyOf(declRefExpr(to(varDecl(
                         hasType(qualType(isConstQualified(), isInteger())),
                         hasInitializer(GetThreadNum)))),
                     GetThreadNum));
      const auto TrueIf =
          ifStmt(hasCondition(binaryOperator(hasOperatorName("=="),
                                             hasEitherOperand(ThreadNum))
                                  .bind("thread-num-cmp")),
                 hasThen(hasDescendant(declRefExpr(equalsBoundNode("dref")))));
      const auto FalseIf =
          ifStmt(hasCondition(binaryOperator(hasOperatorName("!="),
                                             hasEitherOperand(ThreadNum))
                                  .bind("thread-num-cmp")),
                 hasElse(hasDescendant(declRefExpr(equalsBoundNode("dref")))));
      // FIXME: can use traversal to know if there is an ancestor
      const auto MatchResult = match(
          traverse(TK_IgnoreUnlessSpelledInSource,
                   declRefExpr(declRefExpr().bind("dref"),
                               hasAncestor(ifStmt(anyOf(TrueIf, FalseIf))))),
          *DRef, Ctx);
      IsProtected =
          !MatchResult.empty() &&
          (MatchResult[0].getNodeAs<OMPExecutableDirective>("protected") ||
           MatchResult[0].getNodeAs<BinaryOperator>("thread-num-cmp"));
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

  bool TraverseFunctionDecl(FunctionDecl *const FD) {
    if (ParallelContextDepth == 0 && !FD->isTemplated() &&
        !FD->isDependentContext())
      return Base::TraverseFunctionDecl(FD);

    return true;
  }

  bool TraverseCallExpr(CallExpr *const CE) {
    if (ParallelContextDepth == 0)
      return true;

    Base::TraverseStmt(CE->getCallee());
    for (Expr *const Arg : CE->arguments())
      Base::TraverseStmt(Arg);
    auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::is_contained(CallStack, FDecl))
      return true;

    llvm::StringRef FunctionName;
    if (FDecl->getDeclName().isIdentifier())
      FunctionName = FDecl->getName();
    if (FunctionName == "omp_set_lock")
      ++LockedRegionCount;
    else if (FunctionName == "omp_unset_lock")
      --LockedRegionCount;
    CallStack.push_back(FDecl);
    if (FDecl->hasBody()) {
      for (ParmVarDecl *const Param : FDecl->parameters())
        Base::TraverseDecl(Param);
      Base::TraverseFunctionDecl(FDecl);
    }
    CallStack.pop_back();
    return true;
  }

  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *const CE) {
    if (ParallelContextDepth == 0)
      return true;

    Base::TraverseStmt(CE->getCallee());
    for (Expr *const Arg : CE->arguments())
      Base::TraverseStmt(Arg);
    auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::is_contained(CallStack, FDecl))
      return true;

    llvm::StringRef FunctionName;
    if (FDecl->getDeclName().isIdentifier())
      FunctionName = FDecl->getName();
    if (FunctionName == "omp_set_lock")
      ++LockedRegionCount;
    else if (FunctionName == "omp_unset_lock")
      --LockedRegionCount;
    CallStack.push_back(FDecl);
    if (FDecl->hasBody()) {
      for (ParmVarDecl *const Param : FDecl->parameters())
        Base::TraverseDecl(Param);
      Base::TraverseFunctionDecl(FDecl);
    }
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
        !match(
             traverse(
                 TK_IgnoreUnlessSpelledInSource,
                 declRefExpr(
                     Var.bind("var"), unless(hasType(ThreadSafeType)),
                     unless(hasParent(expr(anyOf(
                         arraySubscriptExpr(),
                         cxxOperatorCallExpr(
                             hasOverloadedOperatorName("[]"),
                             hasArgument(0, equalsBoundNode("var"))),
                         unaryOperator(hasOperatorName("&"),
                                       hasUnaryOperand(equalsBoundNode("var")),
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

    const Stmt *EntryPoint =
        CallStack.empty()
            ? State.Directives.DirectiveStack.back()->getStructuredBlock()
            : CallStack.back()->getBody();

    ExprMutationAnalyzer Analyzer(*EntryPoint, Ctx);

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

  llvm::SmallVector<const FunctionDecl *> CallStack;

  // FIXME: support RAII locks, which may mean tracking scopes because destruct
  // expressions don't have their own AST node
  size_t LockedRegionCount = 0;

  size_t ParallelContextDepth = 0;

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
  Finder->addMatcher(translationUnitDecl().bind("TU"), this);
}

void UnprotectedSharedVariableAccessCheck::check(
    const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;

  const auto *const TU = Result.Nodes.getNodeAs<TranslationUnitDecl>("TU");

  Visitor V(Ctx, ThreadSafeTypes, ThreadSafeFunctions);
  V.TraverseTranslationUnitDecl(const_cast<TranslationUnitDecl *>(TU));
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

      if (UnprotectedAcesses.size() < 2 &&
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
