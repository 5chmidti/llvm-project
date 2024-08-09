//===---------- OpenMP.h - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_H

#include "clang/AST/Expr.h"
#include "clang/AST/ExprOpenMP.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Frontend/OpenMP/OMP.h.inc"
#include "llvm/Support/Casting.h"
#include <llvm/ADT/SmallPtrSet.h>

namespace clang {
class OMPExecutableDirective;
class ValueDecl;
} // namespace clang

namespace clang::tidy::openmp {
// NOLINTBEGIN(readability-identifier-naming)
extern const ast_matchers::internal::VariadicDynCastAllOfMatcher<
    OMPClause, OMPReductionClause>
    ompReductionClause;

extern const ast_matchers::internal::VariadicDynCastAllOfMatcher<
    OMPClause, OMPTaskReductionClause>
    ompTaskReductionClause;

extern const ast_matchers::internal::MapAnyOfMatcherImpl<
    OMPClause, OMPFirstprivateClause, OMPPrivateClause, OMPLastprivateClause,
    OMPLinearClause, OMPReductionClause, OMPTaskReductionClause,
    OMPInReductionClause>
    ompPrivatizationClause;

const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPTaskDirective>
    ompTaskDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPAtomicDirective>
    ompAtomicDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPCriticalDirective>
    ompCriticalDirective;
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

AST_POLYMORPHIC_MATCHER_P(
    reducesVariable,
    AST_POLYMORPHIC_SUPPORTED_TYPES(OMPReductionClause, OMPTaskReductionClause,
                                    OMPInReductionClause),
    ast_matchers::internal::Matcher<ValueDecl>, Var) {
  for (const Expr *const ReductionVar : Node.getVarRefs()) {
    if (auto *const ReductionVarRef =
            llvm::dyn_cast<DeclRefExpr>(ReductionVar)) {
      if (Var.matches(*ReductionVarRef->getDecl(), Finder, Builder))
        return true;
    }
  }
  return false;
}

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getSharedVariables(const OMPExecutableDirective *Directive,
                   const ASTContext &Ctx);

template <typename ClauseKind>
llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getCaptureDeclsOf(const clang::OMPExecutableDirective *const Directive) {
  llvm::SmallPtrSet<const clang::ValueDecl *, 4> Decls;
  for (const clang::OMPClause *const Clause : Directive->clauses())
    if (const auto *const CastClause = llvm::dyn_cast<ClauseKind>(Clause))
      for (const auto *const ClauseChild : Clause->children())
        if (const auto Var = llvm::dyn_cast<clang::DeclRefExpr>(ClauseChild);
            Var)
          Decls.insert(Var->getDecl());
  return Decls;
}

template <typename ClauseKind>
llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getMappedDeclsOf(const clang::OMPExecutableDirective *const Directive) {
  llvm::SmallPtrSet<const clang::ValueDecl *, 4> Decls;
  for (const clang::OMPClause *const Clause : Directive->clauses())
    if (const auto *const CastClause = llvm::dyn_cast<ClauseKind>(Clause))
      for (const auto *const ClauseChild : CastClause->children()) {
        if (const auto *const ArraySection =
                llvm::dyn_cast<OMPArraySectionExpr>(ClauseChild)) {
          if (const auto *const Base = ArraySection->getBase())
            if (const auto *const DRef =
                    llvm::dyn_cast<DeclRefExpr>(Base->IgnoreImpCasts()))
              Decls.insert(DRef->getDecl());
        } else if (const auto Var =
                       llvm::dyn_cast<clang::DeclRefExpr>(ClauseChild);
                   Var)
          Decls.insert(Var->getDecl());
      }

  return Decls;
}

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getPrivatizedVariables(const OMPExecutableDirective *Directive,
                       const ASTContext &Ctx);

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getDependVariables(const OMPExecutableDirective *Directive);

bool isUndeferredTask(const OMPExecutableDirective *const Directive,
                      const ASTContext &Ctx);

bool hasBarrier(const OMPExecutableDirective *const Directive,
                const ASTContext &Ctx);

bool isOpenMPDirectiveKind(const OpenMPDirectiveKind DKind,
                           const OpenMPDirectiveKind Expected);

template <typename... OMPDirectiveKinds>
bool isOpenMPDirectiveKind(const OpenMPDirectiveKind Kind,
                           const OMPDirectiveKinds... Expected) {
  const auto LeafConstructs = llvm::omp::getLeafConstructs(Kind);
  return ((Kind == Expected || llvm::is_contained(LeafConstructs, Expected)) ||
          ...);
}

template <typename... ClauseTypes>
bool hasAnyClause(const OMPExecutableDirective *const Directive) {
  return (!Directive->getClausesOfKind<ClauseTypes>().empty() || ...);
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
    if (Iter == Vars.end()) {
      // llvm::errs() << Var->getName() << " is not in Vars\n";
      return true;
    }

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

} // namespace clang::tidy::openmp

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_H
