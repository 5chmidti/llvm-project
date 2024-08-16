//===--- OverlappingDependencyStorageCheck.cpp - clang-tidy ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "OverlappingDependencyStorageCheck.h"
#include "../utils/ASTUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprOpenMP.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/Tooling/FixIt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPTaskDirective>
    ompTaskDirective;
// NOLINTEND(readability-identifier-naming)
} // namespace

class OverlappingDependencyFinder
    : public RecursiveASTVisitor<OverlappingDependencyFinder> {
public:
  using Base = RecursiveASTVisitor<OverlappingDependencyFinder>;

  struct SectionInfo {
    const Expr *E;
    int64_t LowerBound = 0;
    int64_t Length = std::numeric_limits<int64_t>::max();
    int64_t Stride = 1;
  };

  explicit OverlappingDependencyFinder(ClangTidyCheck *Check,
                                       const ASTContext &Ctx)
      : Check(Check), Ctx(Ctx) {}

  ~OverlappingDependencyFinder() { maybeDiagnose(Results.back()); }

  bool analyzeOMPClause(const OMPDependClause *Depend) {
    for (const Expr *Var : Depend->getVarRefs()) {
      Var = Var->IgnoreImplicit();
      if (const auto *const ArraySection =
              llvm::dyn_cast<OMPArraySectionExpr>(Var))
        saveInfo(ArraySection);
      else if (const auto *const DRef = llvm::dyn_cast<DeclRefExpr>(Var))
        saveInfo(DRef);
      else if (const auto *const Subscript =
                   llvm::dyn_cast<ArraySubscriptExpr>(Var))
        saveInfo(Subscript);
    }

    return true;
  }

  bool TraverseOMPTaskDirective(OMPTaskDirective *TD) {
    for (auto *Clause : TD->getClausesOfKind<OMPDependClause>())
      analyzeOMPClause(Clause);

    Results.push_back({});
    Base::TraverseStmt(TD->getStructuredBlock());
    maybeDiagnose(Results.back());
    Results.pop_back();
    return true;
  }

  llvm::SmallVector<std::map<const Expr *, llvm::SmallVector<SectionInfo>>, 2>
      Results{{}};

private:
  void saveInfo(const OMPArraySectionExpr *ArraySection) {
    const Expr *const Base = ArraySection->getBase()->IgnoreParenImpCasts();
    const auto *const DRef = llvm::dyn_cast<DeclRefExpr>(Base);
    if (!DRef)
      return;
    SectionInfo Info;
    Info.E = ArraySection;

    Info.LowerBound = evaluate(ArraySection->getLowerBound()).value_or(0);

    int64_t DefaultLength = 0;
    if (const auto *const Array = llvm::dyn_cast<ConstantArrayType>(
            Base->getType().getCanonicalType().getTypePtr()))
      DefaultLength = Array->getSize().getSExtValue();

    Info.Length = evaluate(ArraySection->getLength()).value_or(DefaultLength);

    Info.Stride = evaluate(ArraySection->getStride()).value_or(1);

    saveInfo(ArraySection->getBase()->IgnoreImplicit(), Info);
  }

  void saveInfo(const DeclRefExpr *DRef) {
    SectionInfo Info;
    Info.E = DRef;

    if (const auto *const Array = llvm::dyn_cast<ConstantArrayType>(
            DRef->getType().getCanonicalType().getTypePtr()))
      Info.Length = Array->getSize().getSExtValue();

    saveInfo(DRef, Info);
  }

  void saveInfo(const ArraySubscriptExpr *Subscript) {
    SectionInfo Info;
    Info.E = Subscript;
    Info.LowerBound = evaluate(Subscript->getIdx()).value_or(-1);
    Info.Length = 1;

    saveInfo(Subscript->getBase()->IgnoreImplicit(), Info);
  }

  void saveInfo(const Expr *E, SectionInfo Info) {
    for (auto &[Key, Value] : Results.back())
      if (utils::areStatementsIdentical(Key, E, Ctx)) {
        Value.push_back(Info);
        return;
      }

    Results.back()[E].push_back(Info);
  }

  void
  maybeDiagnose(const std::map<const Expr *, llvm::SmallVector<SectionInfo>>
                    &SiblingResult) {
    for (const auto &[E, Sections] : SiblingResult)
      for (const auto &[Index, LeftSection] : llvm::enumerate(Sections))
        for (const auto &RightSection : llvm::drop_begin(Sections, Index)) {
          // FIXME: dataflow could invalidate the areStatementsIdentical
          if (LeftSection.E == RightSection.E ||
              utils::areStatementsIdentical(LeftSection.E, RightSection.E, Ctx))
            continue;

          if (LeftSection.LowerBound == RightSection.LowerBound &&
              LeftSection.Length == RightSection.Length)
            continue;

          if ((LeftSection.LowerBound + LeftSection.Length) <=
                  RightSection.LowerBound ||
              (RightSection.LowerBound + RightSection.Length) <=
                  LeftSection.LowerBound)
            continue;

          Check->diag(
              LeftSection.E->getBeginLoc(),
              "the array sections '%0' and '%1' have overlapping storage")
              << LeftSection.E->getSourceRange()
              << RightSection.E->getSourceRange()
              << tooling::fixit::getText(*LeftSection.E, Ctx)
              << tooling::fixit::getText(*RightSection.E, Ctx);
        }
  }

  std::optional<int64_t> evaluate(const Expr *E) {
    if (!E)
      return std::nullopt;
    Expr::EvalResult Result;
    if (E->EvaluateAsInt(Result, Ctx))
      return Result.Val.getInt().getExtValue();
    return std::nullopt;
  }

  ClangTidyCheck *Check;
  const ASTContext &Ctx;
};

void OverlappingDependencyStorageCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(hasDescendant(ompTaskDirective())).bind("func"), this);
}

void OverlappingDependencyStorageCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Func = Result.Nodes.getNodeAs<FunctionDecl>("func");
  OverlappingDependencyFinder Visitor{this, *Result.Context};
  Visitor.TraverseFunctionDecl(const_cast<FunctionDecl *>(Func));
}

} // namespace clang::tidy::openmp
