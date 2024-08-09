//===---------- OpenMP.cpp - clang-tidy -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "OpenMP.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/Support/Casting.h"
#include <cstdint>

static void maybeAdd(llvm::SmallPtrSet<const clang::ValueDecl *, 4> &Decls,
                     const clang::ValueDecl *VDecl) {
  if (VDecl && !VDecl->isImplicit() && !VDecl->getName().starts_with("."))
    Decls.insert(VDecl);
}

template <typename ClauseKind>
void addCapturedDeclsOf(const clang::OMPExecutableDirective *const Directive,
                        llvm::SmallPtrSet<const clang::ValueDecl *, 4> &Decls) {
  for (const clang::OMPClause *const Clause : Directive->clauses())
    if (const auto *const CastClause = llvm::dyn_cast<ClauseKind>(Clause))
      for (const auto *const ClauseChild : Clause->children())
        if (const auto *Var = llvm::dyn_cast<clang::DeclRefExpr>(ClauseChild))
          maybeAdd(Decls, Var->getDecl());
}

template <typename ClauseKind>
void eraseCapturedDeclsOf(
    const clang::OMPExecutableDirective *const Directive,
    llvm::SmallPtrSet<const clang::ValueDecl *, 4> &Decls) {
  for (const clang::OMPClause *const Clause : Directive->clauses())
    if (const auto *const CastClause = llvm::dyn_cast<ClauseKind>(Clause))
      for (const auto *const ClauseChild : Clause->children())
        if (const auto *Var = llvm::dyn_cast<clang::DeclRefExpr>(ClauseChild);
            Var && !Var->refersToEnclosingVariableOrCapture())
          maybeAdd(Decls, Var->getDecl());
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
getSharedVariables(const OMPExecutableDirective *Directive,
                   const ASTContext &Ctx) {
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
            maybeAdd(PossiblySharedDecls, DRef->getDecl());

  eraseCapturedDeclsOf<OMPFirstprivateClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPPrivateClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPLastprivateClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPLinearClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPReductionClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPTaskReductionClause>(Directive, PossiblySharedDecls);
  eraseCapturedDeclsOf<OMPInReductionClause>(Directive, PossiblySharedDecls);

  return PossiblySharedDecls;
}

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getPrivatizedVariables(const OMPExecutableDirective *Directive,
                       const ASTContext &Ctx) {
  llvm::SmallPtrSet<const clang::ValueDecl *, 4> Decls;

  addCapturedDeclsOf<OMPFirstprivateClause>(Directive, Decls);
  addCapturedDeclsOf<OMPPrivateClause>(Directive, Decls);
  addCapturedDeclsOf<OMPLastprivateClause>(Directive, Decls);
  addCapturedDeclsOf<OMPLinearClause>(Directive, Decls);
  addCapturedDeclsOf<OMPReductionClause>(Directive, Decls);
  addCapturedDeclsOf<OMPTaskReductionClause>(Directive, Decls);
  addCapturedDeclsOf<OMPInReductionClause>(Directive, Decls);

  if (const auto *Ordered = Directive->getSingleClause<OMPOrderedClause>()) {
    if (const Expr *NumForLoopsExpr = Ordered->getNumForLoops()) {
      Expr::EvalResult NumForLoopsEval;
      NumForLoopsExpr->EvaluateAsInt(NumForLoopsEval, Ctx);
      const auto NumForLoops = NumForLoopsEval.Val.getInt().getExtValue();
      for (int LoopCounterIndex = 0; LoopCounterIndex < NumForLoops;
           ++LoopCounterIndex)
        if (const auto *DRef = llvm::dyn_cast<DeclRefExpr>(
                Ordered->getLoopCounter(LoopCounterIndex))) {
          Decls.insert(DRef->getDecl());
        }
    }
  }

  if (const auto *Collapse = Directive->getSingleClause<OMPCollapseClause>()) {
    const Expr *NumForLoopsExpr = Collapse->getNumForLoops();
    Expr::EvalResult NumForLoopsEval;
    NumForLoopsExpr->EvaluateAsInt(NumForLoopsEval, Ctx);
    const auto NumForLoops = NumForLoopsEval.Val.getInt().getExtValue();

    class Visitor : public RecursiveASTVisitor<Visitor> {
    public:
      using Base = RecursiveASTVisitor<Visitor>;

      Visitor(int64_t NumForLoops,
              llvm::SmallPtrSet<const clang::ValueDecl *, 4> &Decls)
          : NumForLoops(NumForLoops), Decls(Decls) {}

      bool TraverseForStmt(ForStmt *FS) {
        if (NumForLoops == 0)
          return false;

        if (const auto *BinOp =
                llvm::dyn_cast_if_present<BinaryOperator>(FS->getInit())) {
          if (BinOp->getOpcode() != BinaryOperatorKind::BO_Assign)
            return false;
          const Expr *LHS = BinOp->getLHS();
          const auto *DRef = llvm::dyn_cast<DeclRefExpr>(LHS);
          if (!DRef)
            return false;
          Decls.insert(DRef->getDecl());
        }

        --NumForLoops;
        if (NumForLoops != 0)
          return Base::TraverseForStmt(FS);

        return false;
      }

      int64_t NumForLoops = 0;
      llvm::SmallPtrSet<const clang::ValueDecl *, 4> &Decls;
    };

    Visitor V(NumForLoops, Decls);
    V.TraverseStmt(const_cast<Stmt *>(Directive->getAssociatedStmt()));
  }

  return Decls;
}

llvm::SmallPtrSet<const clang::ValueDecl *, 4>
getDependVariables(const OMPExecutableDirective *Directive) {
  llvm::SmallPtrSet<const clang::ValueDecl *, 4> Decls;

  addCapturedDeclsOf<OMPDependClause>(Directive, Decls);

  return Decls;
}

bool isUndeferredTask(const OMPExecutableDirective *const Directive,
                      const ASTContext &Ctx) {
  const auto *const Task = llvm::dyn_cast<OMPTaskDirective>(Directive);
  if (!Task)
    return false;

  for (const auto *const Clause : Task->clauses()) {
    const auto *const If = llvm::dyn_cast<OMPIfClause>(Clause);
    if (!If)
      continue;
    bool Result = false;
    if (If->getCondition()->EvaluateAsBooleanCondition(Result, Ctx) && !Result)
      return true;
  }
  return false;
}

bool hasBarrier(const OMPExecutableDirective *const Directive,
                const ASTContext &Ctx) {
  const bool HasNowait = hasAnyClause<OMPNowaitClause>(Directive);
  if (llvm::isa<OMPBarrierDirective, OMPTaskwaitDirective,
                OMPTaskgroupDirective>(Directive) ||
      (llvm::isa<OMPSingleDirective>(Directive) && !HasNowait) ||
      isUndeferredTask(Directive, Ctx))
    return true;

  if (hasAnyClause<OMPNowaitClause, OMPCopyprivateClause>(Directive))
    return false;

  if (isOpenMPDirectiveKind(
          Directive->getDirectiveKind(), OpenMPDirectiveKind::OMPD_parallel,
          OpenMPDirectiveKind::OMPD_scope, OpenMPDirectiveKind::OMPD_sections,
          OpenMPDirectiveKind::OMPD_workshare, OpenMPDirectiveKind::OMPD_for))
    return true;

  return false;
}

bool isOpenMPDirectiveKind(const OpenMPDirectiveKind DKind,
                           const OpenMPDirectiveKind Expected) {
  return DKind == Expected ||
         llvm::is_contained(getLeafConstructs(DKind), Expected);
}
} // namespace clang::tidy::openmp
