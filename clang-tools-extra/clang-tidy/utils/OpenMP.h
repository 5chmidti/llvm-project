//===---------- OpenMP.h - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_H

#include "clang/AST/OpenMPClause.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include <llvm/ADT/SmallPtrSet.h>

namespace clang {
class OMPExecutableDirective;
class ValueDecl;
} // namespace clang

namespace clang::tidy::openmp {
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
  if (const auto *const Clause =
          Directive->template getSingleClause<ClauseKind>())
    for (const auto *const ClauseChild : Clause->children())
      if (const auto Var = llvm::dyn_cast<clang::DeclRefExpr>(ClauseChild);
          Var && !Var->refersToEnclosingVariableOrCapture())
        Decls.insert(Var->getDecl());
  return Decls;
}

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getPrivatizedVariables(const OMPExecutableDirective *Directive);

struct SharedAndPrivateVariables {
llvm::SmallPtrSet<const clang::ValueDecl *, 4> Shared;
llvm::SmallPtrSet<const clang::ValueDecl *, 4> Private;
};
SharedAndPrivateVariables
getSharedAndPrivateVariable(const OMPExecutableDirective *Directive);

} // namespace clang::tidy::openmp

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_H
