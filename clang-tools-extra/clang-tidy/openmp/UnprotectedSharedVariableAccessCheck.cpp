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
#include "llvm/ADT/StringRef.h"
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
#include <llvm/ADT/SetOperations.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Frontend/OpenMP/OMPConstants.h>
#include <llvm/Support/Casting.h>
#include <optional>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::MapAnyOfMatcher<
    OMPCriticalDirective, OMPAtomicDirective, OMPOrderedDirective,
    OMPMasterDirective, OMPSingleDirective, OMPAtomicDirective,
    OMPFlushDirective>
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

AST_POLYMORPHIC_MATCHER_P(
    reducesVariable,
    AST_POLYMORPHIC_SUPPORTED_TYPES(OMPReductionClause, OMPTaskReductionClause),
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

class UnprotectedSharedVariableAccessAnalyzer : private ExprMutationAnalyzer {
public:
  UnprotectedSharedVariableAccessAnalyzer(const Stmt &Stm, ASTContext &Context)
      : ExprMutationAnalyzer(Stm, Context), Stm(Stm), Context(Context) {}

  llvm::SmallVector<const Stmt *, 4>
  findAllMutations(const ValueDecl *const Dec,
                   const llvm::ArrayRef<llvm::StringRef> &ThreadSafeTypes,
                   const llvm::ArrayRef<llvm::StringRef> &ThreadSafeFunctions) {
    const auto ThreadSafeType = qualType(anyOf(
        atomicType(),
        qualType(matchers::matchesAnyListedTypeName(ThreadSafeTypes)),
        hasCanonicalType(matchers::matchesAnyListedTypeName(ThreadSafeTypes))));

    if (!match(ThreadSafeType, Dec->getType(), Context).empty()) {
      return {};
    }

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

    const auto IsAReductionVariable = hasAncestor(ompExecutableDirective(anyOf(
        hasAnyClause(ompReductionClause(reducesVariable(equalsNode(Dec)))),
        hasAnyClause(
            ompTaskReductionClause(reducesVariable(equalsNode(Dec)))))));

    const auto Refs = match(
        findAll(traverse(
                    TK_IgnoreUnlessSpelledInSource,
                    declRefExpr(
                        Var,
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
                            IsCastToRValueOrConst, AtomicIntrinsicCall)))),
                        unless(IsAReductionVariable)))
                    .bind("expr")),
        Stm, Context);

    auto Mutations = llvm::SmallVector<const Stmt *, 4>{};
    for (const auto &RefNodes : Refs) {
      const auto *const E = RefNodes.getNodeAs<Expr>("expr");
      if (isMutated(E))
        Mutations.push_back(E);
    }
    return Mutations;
  }

private:
  const Stmt &Stm;
  ASTContext &Context;
};

llvm::SmallVector<const ValueDecl *, 4>
getIterationVariablesOfDirective(const OMPExecutableDirective *const Directive,
                                 const size_t NumLoopsToCheck,
                                 ASTContext &Ctx) {
  llvm::SmallVector<const DeclRefExpr *, 4> Result{};
  if (const auto *const LoopDirective =
          llvm::dyn_cast<OMPLoopDirective>(Directive)) {
    llvm::SmallVector<const ValueDecl *, 4> Result{};
    for (const Expr *const E : LoopDirective->counters())
      if (const auto *const DRef = llvm::dyn_cast_if_present<DeclRefExpr>(E))
        Result.push_back(DRef->getDecl());
    return Result;
  }
  return {};
}

std::optional<size_t> getOptCollapseNum(const OMPCollapseClause *const Collapse,
                                        const ASTContext &Ctx) {
  if (!Collapse)
    return std::nullopt;
  const Expr *const E = Collapse->getNumForLoops();
  const std::optional<llvm::APSInt> OptInt = E->getIntegerConstantExpr(Ctx);
  if (OptInt)
    return OptInt->tryExtValue();
  return std::nullopt;
}

const auto DefaultThreadSafeTypes = "std::atomic; std::atomic_ref";
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

void UnprotectedSharedVariableAccessCheck::checkSharedVariable(
    const ValueDecl *const SharedVar, const Stmt *const Scope, ASTContext &Ctx,
    const llvm::SmallVector<const ValueDecl *, 4> &LoopVars) {
  const auto SharedType = SharedVar->getType();
  if (const auto *const RDecl = SharedType->getAsRecordDecl())
    if (RDecl->getName().contains("map"))
      return;

  const auto UnprotectedAcesses = match(
      findAll(declRefExpr(to(equalsNode(SharedVar)),
                          unless(hasAncestor(ompProtectedAccessDirective())))
                  .bind("decl_ref")),
      *Scope, Ctx);

  if (UnprotectedAcesses.empty()) {
    return;
  }

  auto MutationAnalyzer = UnprotectedSharedVariableAccessAnalyzer(*Scope, Ctx);

  const auto Mutations = MutationAnalyzer.findAllMutations(
      SharedVar, ThreadSafeTypes, ThreadSafeFunctions);
  if (Mutations.empty()) {
    return;
  }

  for (const auto &UnprotectedAcessNodes : UnprotectedAcesses) {
    const auto *const UnprotectedAccess =
        UnprotectedAcessNodes.getNodeAs<DeclRefExpr>("decl_ref");
    diag(UnprotectedAccess->getBeginLoc(),
         "do not access shared variable %0 of type %1 without synchronization")
        << UnprotectedAccess->getSourceRange() << SharedVar
        << SharedVar->getType();
    for (const Stmt *const Mutation : Mutations)
      diag(Mutation->getBeginLoc(), "%0 was mutated here",
           DiagnosticIDs::Level::Note)
          << Mutation->getSourceRange() << SharedVar;
  }
}

void UnprotectedSharedVariableAccessCheck::check(
    const MatchFinder::MatchResult &Result) {
  ASTContext &Ctx = *Result.Context;

  const auto *const Directive =
      Result.Nodes.getNodeAs<OMPExecutableDirective>("directive");

  const Stmt *const StructuredBlock = Directive->getStructuredBlock();

  const auto *const CollapseClause =
      Directive->getSingleClause<OMPCollapseClause>();
  const auto NumLoopsToCheck =
      getOptCollapseNum(CollapseClause, Ctx).value_or(1U);

  const auto LoopIterVars =
      getIterationVariablesOfDirective(Directive, NumLoopsToCheck, Ctx);

  if (Directive->getNumClauses() == 0) {
    // If no clause is present, sharing is enabled by default, and we need to
    // check all captured `DeclRefExpr`s.
    for (const auto *const Child : Directive->children())
      if (const auto *const Capture = llvm::dyn_cast<CapturedStmt>(Child))
        for (const auto InnerCapture : Capture->captures())
          if (InnerCapture.getCaptureKind() == CapturedStmt::VCK_ByRef)
            checkSharedVariable(InnerCapture.getCapturedVar(), StructuredBlock,
                                Ctx, LoopIterVars);
    return;
  }

  const auto SharedDecls = openmp::getSharedVariables(Directive);

  for (const ValueDecl *const SharedDecl : SharedDecls)
    if (match(valueDecl(hasType(qualType(anyOf(
                  atomicType(),
                  hasCanonicalType(hasDeclaration(namedDecl(
                      matchers::matchesAnyListedName(ThreadSafeTypes)))))))),
              *SharedDecl, Ctx)
            .empty())
      checkSharedVariable(SharedDecl, StructuredBlock, Ctx, LoopIterVars);
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
