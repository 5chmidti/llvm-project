//===--- DeclareLoopIterationVariableInForInitStatementCheck.cpp - clang-tidy
//-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DeclareLoopIterationVariableInForInitStatementCheck.h"
#include "../utils/LexerUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/ADT/APSInt.h"
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
using internal::Matcher;
using internal::VariadicDynCastAllOfMatcher;

// NOLINTBEGIN(readability-identifier-naming)
const VariadicDynCastAllOfMatcher<Stmt, OMPLoopDirective> ompLoopDirective;
const VariadicDynCastAllOfMatcher<OMPClause, OMPCollapseClause>
    ompCollapseClause;
// NOLINTEND(readability-identifier-naming)

AST_MATCHER_P(OMPExecutableDirective, hasAssociatedStmt, Matcher<Stmt>,
              InnerMatcher) {
  const Stmt *const S = Node.getAssociatedStmt();
  return S && InnerMatcher.matches(*S, Finder, Builder);
}

AST_MATCHER_P(Stmt, ignoringContainers, Matcher<Stmt>, InnerMatcher) {
  return InnerMatcher.matches(*Node.IgnoreContainers(true), Finder, Builder);
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

std::string
getValueDeclsList(const std::vector<const ValueDecl *> &IterVarDecls) {
  std::string Str{};
  for (size_t I = 0U; I < IterVarDecls.size(); ++I) {
    if (I != 0U && I != IterVarDecls.size() - 1)
      Str += ", ";
    else if (I != 0 && I == IterVarDecls.size() - 1)
      Str += " and ";
    Str += "'" + IterVarDecls[I]->getNameAsString() + "'";
  }
  return Str;
}

llvm::SmallVector<const DeclRefExpr *, 4>
getIterationVariableOfDirectiveForStmts(const Stmt *const FirstForLike,
                                        const size_t NumLoopsToCheck,
                                        ASTContext &Ctx) {
  class IterVarsOfRawForStmtsFromCollapsedLoopDirective
      : public RecursiveASTVisitor<
            IterVarsOfRawForStmtsFromCollapsedLoopDirective> {
  public:
    IterVarsOfRawForStmtsFromCollapsedLoopDirective(size_t NumLoopsToCheck,
                                                    ASTContext &Ctx)
        : NumLoopsToCheck(NumLoopsToCheck), Ctx(Ctx) {}

    bool VisitForStmt(ForStmt *For) {
      const auto MatchResult = match(AssignOnlyLoopVar, *For->getInit(), Ctx);
      for (const auto &V : MatchResult)
        Result.push_back(V.getNodeAs<DeclRefExpr>("dref"));

      --NumLoopsToCheck;
      return NumLoopsToCheck != 0U;
    }

    bool VisitCXXForRangeStmt(CXXForRangeStmt *For) {
      --NumLoopsToCheck;
      return NumLoopsToCheck != 0U;
    }

    size_t NumLoopsToCheck;
    llvm::SmallVector<const DeclRefExpr *, 4> Result{};
    ASTContext &Ctx;
    ast_matchers::internal::BindableMatcher<Stmt> AssignOnlyLoopVar =
        binaryOperator(isAssignmentOperator(),
                       hasLHS(declRefExpr().bind("dref")));
  };

  IterVarsOfRawForStmtsFromCollapsedLoopDirective Visitor{NumLoopsToCheck, Ctx};
  if (FirstForLike)
    Visitor.TraverseStmt(const_cast<Stmt *>(FirstForLike));

  return Visitor.Result;
}
} // namespace

void DeclareLoopIterationVariableInForInitStatementCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      ompLoopDirective(
          hasAssociatedStmt(ignoringContainers(
              stmt(anyOf(forStmt(), cxxForRangeStmt())).bind("for-like"))),
          optionally(hasAnyClause(ompCollapseClause().bind("collapse"))))
          .bind("directive"),
      this);
}

void DeclareLoopIterationVariableInForInitStatementCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Directive =
      Result.Nodes.getNodeAs<OMPLoopDirective>("directive");
  const auto *const Collapse =
      Result.Nodes.getNodeAs<OMPCollapseClause>("collapse");
  const auto *const ForLike = Result.Nodes.getNodeAs<Stmt>("for-like");
  const std::optional<size_t> OptCollapseNum =
      getOptCollapseNum(Collapse, *Result.Context);

  const llvm::SmallVector<const DeclRefExpr *, 4> IterVarsNotDeclaredInInit =
      getIterationVariableOfDirectiveForStmts(
          ForLike, OptCollapseNum.value_or(1U), *Result.Context);

  if (IterVarsNotDeclaredInInit.empty())
    return;

  std::vector<const ValueDecl *> IterVarDecls{};

  for (const DeclRefExpr *const DRef : IterVarsNotDeclaredInInit) {
    IterVarDecls.push_back(DRef->getDecl());
  }

  {
    auto Diag = diag(Directive->getBeginLoc(),
                     "declare the loop iteration "
                     "%plural{1:variable|:variables}0 %1 in the "
                     "for-loop initializer %plural{1:statement|:statements}0")
                << Directive->getSourceRange() << IterVarDecls.size()
                << getValueDeclsList(IterVarDecls);
    for (const DeclRefExpr *const DRef : IterVarsNotDeclaredInInit)
      Diag << DRef->getSourceRange();
  }

  for (const ValueDecl *const IterVar : IterVarDecls)
    diag(IterVar->getLocation(), "%0 was declared here",
         DiagnosticIDs::Level::Note)
        << IterVar->getSourceRange() << IterVar;
}

} // namespace clang::tidy::openmp
