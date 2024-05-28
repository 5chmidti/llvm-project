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
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/ADT/STLExtras.h"
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

const ast_matchers::internal::MapAnyOfMatcher<
    OMPCriticalDirective, OMPAtomicDirective, OMPOrderedDirective,
    OMPSingleDirective> // FIXME: single is not protected, what about ordered?
                        // how to fit masked into this?
    ompProtectedAccessDirective;

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
getSharedVariables(const OMPExecutableDirective *Directive);

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
getPrivatizedVariables(const OMPExecutableDirective *Directive);

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

} // namespace clang::tidy::openmp

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_H
