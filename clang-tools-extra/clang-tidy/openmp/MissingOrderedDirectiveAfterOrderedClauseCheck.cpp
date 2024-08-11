//===--- MissingOrderedDirectiveAfterOrderedClauseCheck.cpp - clang-tidy --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MissingOrderedDirectiveAfterOrderedClauseCheck.h"
#include "clang/AST/Decl.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include <vector>

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
// NOLINTBEGIN(readability-identifier-naming)
const ast_matchers::internal::VariadicDynCastAllOfMatcher<Stmt,
                                                          OMPOrderedDirective>
    ompOrderedDirective;
const ast_matchers::internal::VariadicDynCastAllOfMatcher<OMPClause,
                                                          OMPOrderedClause>
    ompOrderedClause;
// NOLINTEND(readability-identifier-naming)
} // namespace

class Visitor : public RecursiveASTVisitor<Visitor> {
public:
  using Base = RecursiveASTVisitor<Visitor>;

  explicit Visitor(ClangTidyCheck *Check) : Check(Check) {}

  bool TraverseOMPExecutableDirective(OMPExecutableDirective *Directive) {
    if (Directive->isStandaloneDirective())
      return true;

    const auto HasOrderedDirective =
        Directive->getSingleClause<OMPOrderedClause>() != nullptr;

    if (HasOrderedDirective)
      Stack.push_back(false);
    Base::TraverseStmt(Directive->getStructuredBlock());
    if (HasOrderedDirective && !Stack.back()) {
      Check->diag(Directive->getBeginLoc(),
                  "this '%0' directive has an 'ordered' clause "
                  "but the associated statement "
                  "has no 'ordered' directive")
          << getOpenMPDirectiveName(Directive->getDirectiveKind())
          << Directive->getSourceRange();
    }

    return true;
  }

  bool TraverseOMPOrderedDirective(OMPOrderedDirective *Directive) {
    if (!Stack.empty())
      Stack.back() = true;

    return true;
  }

  bool TraverseCallExpr(CallExpr *const CE) {
    if (Stack.empty())
      return true;

    Base::TraverseStmt(CE->getCallee());
    for (Expr *const Arg : CE->arguments())
      Base::TraverseStmt(Arg);
    auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::is_contained(CallStack, FDecl))
      return true;

    llvm::StringRef FunctionName;
    if (FDecl->getDeclName().isIdentifier())
      FunctionName = FDecl->getName();

    CallStack.push_back(FDecl);
    if (FDecl->hasBody()) {
      for (ParmVarDecl *const Param : FDecl->parameters())
        Base::TraverseDecl(Param);
      Base::TraverseFunctionDecl(FDecl);
    }
    CallStack.pop_back();
    return true;
  }

  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *const CE) {
    if (Stack.empty())
      return true;

    Base::TraverseStmt(CE->getCallee());
    for (Expr *const Arg : CE->arguments())
      Base::TraverseStmt(Arg);
    auto *const FDecl =
        llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
    if (!FDecl)
      return true;
    if (llvm::is_contained(CallStack, FDecl))
      return true;

    llvm::StringRef FunctionName;
    if (FDecl->getDeclName().isIdentifier())
      FunctionName = FDecl->getName();

    CallStack.push_back(FDecl);
    if (FDecl->hasBody()) {
      for (ParmVarDecl *const Param : FDecl->parameters())
        Base::TraverseDecl(Param);
      Base::TraverseFunctionDecl(FDecl);
    }
    CallStack.pop_back();
    return true;
  }

  ClangTidyCheck *Check;
  std::vector<bool> Stack{};
  llvm::SmallVector<const FunctionDecl *> CallStack;
};

void MissingOrderedDirectiveAfterOrderedClauseCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(translationUnitDecl().bind("TU"), this);
}

void MissingOrderedDirectiveAfterOrderedClauseCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *TU = Result.Nodes.getNodeAs<TranslationUnitDecl>("TU");

  Visitor Vis{this};
  Vis.TraverseTranslationUnitDecl(const_cast<TranslationUnitDecl *>(TU));
}

} // namespace clang::tidy::openmp
