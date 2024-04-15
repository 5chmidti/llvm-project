//===---------- OpenMP.cpp - clang-tidy -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "OpenMP.h"
#include "clang/AST/Decl.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/Support/Casting.h"

template <typename ClauseKind>
void addCapturedDeclsOf(const clang::OMPExecutableDirective *const Directive,
                        llvm::SmallPtrSet<const clang::ValueDecl *, 4> &Decls) {
  for (const clang::OMPClause *const Clause : Directive->clauses())
    if (const auto *const CastClause = llvm::dyn_cast<ClauseKind>(Clause))
      for (const auto *const ClauseChild : Clause->children())
        if (const auto Var = llvm::dyn_cast<clang::DeclRefExpr>(ClauseChild))
          Decls.insert(Var->getDecl());
}

template <typename ClauseKind>
void eraseCapturedDeclsOf(
    const clang::OMPExecutableDirective *const Directive,
    llvm::SmallPtrSet<const clang::ValueDecl *, 4> &Decls) {
  for (const clang::OMPClause *const Clause : Directive->clauses())
    if (const auto *const CastClause = llvm::dyn_cast<ClauseKind>(Clause))
      for (const auto *const ClauseChild : Clause->children())
        if (const auto Var = llvm::dyn_cast<clang::DeclRefExpr>(ClauseChild);
            Var && !Var->refersToEnclosingVariableOrCapture())
          Decls.erase(Var->getDecl());
}

namespace clang::tidy::openmp {
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPReductionClause>
    ompReductionClause;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<
    OMPClause, OMPTaskReductionClause>
    ompTaskReductionClause;

const ast_matchers::internal::MapAnyOfMatcherImpl<
    OMPClause, OMPFirstprivateClause, OMPPrivateClause, OMPLastprivateClause,
    OMPLinearClause, OMPReductionClause, OMPTaskReductionClause,
    OMPInReductionClause>
    ompPrivatizationClause;

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getSharedVariables(const OMPExecutableDirective *Directive) {
  if (!isOpenMPParallelDirective(Directive->getDirectiveKind()) &&
      !isOpenMPTaskingDirective(Directive->getDirectiveKind()))
    return {};

  llvm::SmallPtrSet<const ValueDecl *, 4> PossiblySharedDecls{};

  // The OMPDefaultClause does not provide a way to know which decls get
  // captured, therefore, we add all captured decls to DeclsToCheck and remove
  // all decls that are from clause that privatize variables.

  addCapturedDeclsOf<OMPSharedClause>(Directive, PossiblySharedDecls);
  if (const auto *const DefaultClause =
          Directive->getSingleClause<OMPDefaultClause>();
      !DefaultClause || DefaultClause->getDefaultKind() ==
                            llvm::omp::DefaultKind::OMP_DEFAULT_shared)
    for (const auto *const Child : Directive->children())
      if (const auto *const Capture = llvm::dyn_cast<CapturedStmt>(Child))
        for (const auto *const InnerCapture : Capture->children())
          if (const auto *const DRef =
                  llvm::dyn_cast_if_present<DeclRefExpr>(InnerCapture);
              DRef && !DRef->refersToEnclosingVariableOrCapture())
            PossiblySharedDecls.insert(DRef->getDecl());

  eraseCapturedDeclsOf<OMPFirstprivateClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPPrivateClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPLastprivateClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPLinearClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPReductionClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPTaskReductionClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPInReductionClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPDependClause>(Directive, PossiblySharedDecls);

  return PossiblySharedDecls;
}

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getPrivatizedVariables(const OMPExecutableDirective *Directive) {
  llvm::SmallPtrSet<const clang::ValueDecl *, 4> Decls;

  addCapturedDeclsOf<OMPFirstprivateClause>(Directive, Decls);
  addCapturedDeclsOf<OMPPrivateClause>(Directive, Decls);
  addCapturedDeclsOf<OMPLastprivateClause>(Directive, Decls);
  addCapturedDeclsOf<OMPLinearClause>(Directive, Decls);
  addCapturedDeclsOf<OMPReductionClause>(Directive, Decls);
  addCapturedDeclsOf<OMPTaskReductionClause>(Directive, Decls);
  addCapturedDeclsOf<OMPInReductionClause>(Directive, Decls);
  addCapturedDeclsOf<OMPDependClause>(Directive, Decls);

  return Decls;
}

SharedAndPrivateVariables
getSharedAndPrivateVariable(const OMPExecutableDirective *Directive) {
  return {getSharedVariables(Directive), getPrivatizedVariables(Directive)};
}
} // namespace clang::tidy::openmp
