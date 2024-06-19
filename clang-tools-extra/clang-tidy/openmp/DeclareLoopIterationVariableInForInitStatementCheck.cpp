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
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersInternal.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/Support/Casting.h"
#include <cstddef>
#include <string>
#include <vector>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
using internal::Matcher;
using internal::VariadicDynCastAllOfMatcher;

// NOLINTBEGIN(readability-identifier-naming)
const VariadicDynCastAllOfMatcher<Stmt, OMPLoopDirective> ompLoopDirective;
// NOLINTEND(readability-identifier-naming)

AST_MATCHER_P(OMPExecutableDirective, hasAssociatedStmt, Matcher<Stmt>,
              InnerMatcher) {
  const Stmt *const S = Node.getAssociatedStmt();
  return S && InnerMatcher.matches(*S, Finder, Builder);
}

AST_MATCHER_P(Stmt, ignoringContainers, Matcher<Stmt>, InnerMatcher) {
  return InnerMatcher.matches(*Node.IgnoreContainers(true), Finder, Builder);
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
} // namespace

void DeclareLoopIterationVariableInForInitStatementCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      ompLoopDirective(
          hasAssociatedStmt(ignoringContainers(
              stmt(anyOf(forStmt(), cxxForRangeStmt())).bind("for-like"))))
          .bind("directive"),
      this);
}

void DeclareLoopIterationVariableInForInitStatementCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const Directive =
      Result.Nodes.getNodeAs<OMPLoopDirective>("directive");

  llvm::SmallVector<const DeclRefExpr *, 4> IterVarsNotDeclaredInInit{};
  auto &Ctx = *Result.Context;

  const auto AddVarIfDeclaredBeforeForStmt =
      [&IterVarsNotDeclaredInInit, &Ctx, Directive](const Expr *const Counter) {
        if (!Counter)
          return;

        if (const auto *const DRef = llvm::dyn_cast<DeclRefExpr>(Counter)) {
          const ValueDecl *const Var = DRef->getDecl();
          // Ignore implicitly generated iteration variables by OpenMP
          if (Var->isImplicit())
            return;

          // Ignore variables declared in the for-statement head
          const auto Matches = match(
              ompExecutableDirective(
                  hasDescendant(forStmt(hasLoopInit(declStmt(hasSingleDecl(
                      valueDecl(hasName(Var->getName())).bind("var"))))))),
              *Directive, Ctx);
          if (Matches.empty())
            IterVarsNotDeclaredInInit.push_back(DRef);
        }
      };

  for (const Expr *const Counter : Directive->counters())
    AddVarIfDeclaredBeforeForStmt(Counter);
  for (const Expr *E : Directive->dependent_counters())
    AddVarIfDeclaredBeforeForStmt(E);

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

  const auto *const LastPrivate =
      Directive->getSingleClause<OMPLastprivateClause>();

  if (!LastPrivate)
    return;

  const auto LastPrivateVars = LastPrivate->getVarRefs();

  for (const DeclRefExpr *const DRef : IterVarsNotDeclaredInInit) {
    for (const Expr *const LastPrivateVar : LastPrivateVars)
      if (const auto *const LastPrivateDRefExpr =
              llvm::dyn_cast_or_null<DeclRefExpr>(LastPrivateVar);
          LastPrivateDRefExpr &&
          DRef->getDecl() == LastPrivateDRefExpr->getDecl())
        diag(LastPrivateVar->getBeginLoc(),
             "%0 has been declared as 'lastprivate'",
             DiagnosticIDs::Level::Note)
            << LastPrivateDRefExpr->getDecl()
            << LastPrivateVar->getSourceRange();
  }
}

} // namespace clang::tidy::openmp
