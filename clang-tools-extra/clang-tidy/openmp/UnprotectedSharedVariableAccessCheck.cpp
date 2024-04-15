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
    OMPMasterDirective, OMPSingleDirective, OMPFlushDirective>
    ompProtectedAccessDirective;
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

class Visitor : public RecursiveASTVisitor<Visitor> {
public:
  Visitor(ASTContext &Ctx, llvm::ArrayRef<llvm::StringRef> ThreadSafeTypes,
          llvm::ArrayRef<llvm::StringRef> ThreadSafeFunctions)
      : Ctx(Ctx), ThreadSafeTypes(ThreadSafeTypes),
        ThreadSafeFunctions(ThreadSafeFunctions) {}

  using Base = RecursiveASTVisitor<Visitor>;
  struct AnalysisResult {
    llvm::SmallPtrSet<const DeclRefExpr *, 4> Mutations;
    llvm::SmallPtrSet<const DeclRefExpr *, 4> UnprotectedAcesses;
    llvm::SmallPtrSet<const DeclRefExpr *, 4> UnprotectedDependentAcesses;
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

      SharedVarsSTack.push_back(SharedAndPrivateVars.Shared);
      PrivatizedVarsStack.push_back(PrivatizedVariables);
      DependentVarsSTack.push_back(SharedAndPrivateVars.Dependent);
      CurrentDependentVariables = DependentVarsSTack.back();
      DirectiveStack.push_back(Directive);
    }

    void pop() {
      CurrentSharedVariables.append(PrivatizedVarsStack.back());
      PrivatizedVarsStack.pop_back();
      const llvm::SmallPtrSet<const ValueDecl *, 4> &LatestSharedScopeChange =
          SharedVarsSTack.back();
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
      SharedVarsSTack.pop_back();
      DirectiveStack.pop_back();
      DependentVarsSTack.pop_back();

      if (!DependentVarsSTack.empty())
        CurrentDependentVariables = DependentVarsSTack.back();
    }

    bool isSharedOrDependent(const DeclRefExpr *DRef) const {
      const ValueDecl *Var = DRef->getDecl();
      return isShared(Var) || isDependent(Var);
    }

    bool isShared(const ValueDecl *Var) const {
      return llvm::find_if(CurrentSharedVariables,
                           [Var](const auto &SharedVarWithCount) {
                             return SharedVarWithCount.first == Var;
                           }) != CurrentSharedVariables.end();
    }

    bool isDependent(const ValueDecl *Var) const {
      return llvm::find(CurrentDependentVariables, Var) !=
             CurrentDependentVariables.end();
    }

    // private:
    llvm::SmallVector<std::pair<const ValueDecl *, size_t>>
        CurrentSharedVariables;
    llvm::SmallPtrSet<const ValueDecl *, 4> CurrentDependentVariables;
    llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>> SharedVarsSTack;
    llvm::SmallVector<llvm::SmallVector<std::pair<const ValueDecl *, size_t>>>
        PrivatizedVarsStack;
    llvm::SmallVector<llvm::SmallPtrSet<const ValueDecl *, 4>>
        DependentVarsSTack;
    llvm::SmallVector<const OMPExecutableDirective *> DirectiveStack;
  };

  bool TraverseOMPExecutableDirective(OMPExecutableDirective *Directive) {
    if (Directive->isStandaloneDirective()) {
      if (const auto *const Barrier =
              llvm::dyn_cast<OMPBarrierDirective>(Directive)) {
        saveAnalysisAndStartNewEpoch();
      }
      if (const auto *const Taskwait =
              llvm::dyn_cast<OMPTaskwaitDirective>(Directive)) {
        auto Clauses = Taskwait->clauses();
        if (Clauses.empty()) {
          saveAnalysisAndStartNewEpoch();
          return true;
        }
        for (const OMPClause *const Clause : Clauses) {
          if (const auto *const Depend =
                  llvm::dyn_cast<OMPDependClause>(Clause)) {
            for (const auto *const VarExpr : Depend->getVarRefs()) {
              if (const auto *const DRef = llvm::dyn_cast<DeclRefExpr>(VarExpr))
                saveAnalysisAndStartNewEpoch(DRef->getDecl());
            }
          }
        }
      }
      return true;
    }

    State.add(Directive);
    Stmt *Statement = Directive->getStructuredBlock();
    Base::TraverseStmt(Statement);
    State.pop();
    return true;
  }

  bool TraverseCapturedStmt(CapturedStmt *S) { return true; }

  bool TraverseDeclRefExpr(DeclRefExpr *DRef) {
    if (!State.isSharedOrDependent(DRef))
      return true;

    // FIXME: can use traversal to know if there is an ancestor
    const bool IsUnprotected =
        match(declRefExpr(hasAncestor(ompProtectedAccessDirective())), *DRef,
              Ctx)
            .empty();

    const bool IsDependent =
        llvm::find(State.CurrentDependentVariables, DRef->getDecl()) !=
        State.CurrentDependentVariables.end();

    if (IsUnprotected) {
      if (IsDependent)
        ContextResults[DRef->getDecl()].UnprotectedDependentAcesses.insert(
            DRef);
      else
        ContextResults[DRef->getDecl()].UnprotectedAcesses.insert(DRef);
    }

    if (isMutating(DRef))
      ContextResults[DRef->getDecl()].Mutations.insert(DRef);

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

  llvm::SmallVector<
      std::pair<const clang::ValueDecl *, Visitor::AnalysisResult>>
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
      std::pair<const clang::ValueDecl *, Visitor::AnalysisResult>>
      Results;

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
      ompExecutableDirective(unless(isStandaloneDirective()),
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

    for (const auto &UnprotectedAcessNodes : UnprotectedAcesses) {
      const auto *const UnprotectedAccess =
          dyn_cast<DeclRefExpr>(UnprotectedAcessNodes);
      diag(
          UnprotectedAccess->getBeginLoc(),
          "do not access shared variable %0 of type %1 without synchronization")
          << UnprotectedAccess->getSourceRange() << SharedVar
          << SharedVar->getType();
      for (const Stmt *const Mutation : Mutations)
        diag(Mutation->getBeginLoc(), "%0 was mutated here",
             DiagnosticIDs::Level::Note)
            << Mutation->getSourceRange() << SharedVar;
    }

    // UnprotectedAcesses should be printed unconditionally, but
    // UnprotectedDependentAccesses should only be diagnosed if there exists an
    // unprotected access outside of a dependent region.
    if (UnprotectedAcesses.empty())
      continue;

    for (const auto &UnprotectedAcessNodes : UnprotectedDependentAccesses) {
      const auto *const UnprotectedAccess =
          dyn_cast<DeclRefExpr>(UnprotectedAcessNodes);
      diag(
          UnprotectedAccess->getBeginLoc(),
          "do not access shared variable %0 of type %1 without synchronization")
          << UnprotectedAccess->getSourceRange() << SharedVar
          << SharedVar->getType();
      for (const Stmt *const Mutation : Mutations)
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
