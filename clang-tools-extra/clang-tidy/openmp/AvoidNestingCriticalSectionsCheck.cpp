//===--- AvoidNestingCriticalSectionsCheck.cpp - clang-tidy ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AvoidNestingCriticalSectionsCheck.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/OpenMPKinds.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"

using namespace clang::ast_matchers;

namespace clang::tidy::openmp {
namespace {
using internal::VariadicDynCastAllOfMatcher;

// NOLINTBEGIN(readability-identifier-naming)
const VariadicDynCastAllOfMatcher<Stmt, OMPCriticalDirective>
    ompCriticalDirective;
// NOLINTEND(readability-identifier-naming)

class InnerCriticalDirectiveFinder
    : public RecursiveASTVisitor<InnerCriticalDirectiveFinder> {
public:
  bool TraverseOMPExecutableDirective(OMPExecutableDirective *Directive) {
    return !isOpenMPParallelDirective(Directive->getDirectiveKind());
  }

  bool TraverseCallExpr(CallExpr *CE) {
    if (const auto *const FDecl =
            llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
        FDecl && FDecl->hasBody())
      TraverseStmt(FDecl->getBody());
    return true;
  }

  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *CE) {
    if (const auto *const FDecl =
            llvm::dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
        FDecl && FDecl->hasBody())
      TraverseStmt(FDecl->getBody());
    return true;
  }

  bool VisitOMPCriticalDirective(OMPCriticalDirective *Critical) {
    InnerCriticalDirectives.push_back(Critical);
    return true;
  }

  llvm::SmallVector<const OMPCriticalDirective *> InnerCriticalDirectives;
};

} // namespace

void AvoidNestingCriticalSectionsCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(ompCriticalDirective().bind("critical_outer"), this);
}

void AvoidNestingCriticalSectionsCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *const OuterCritical =
      Result.Nodes.getNodeAs<OMPCriticalDirective>("critical_outer");

  InnerCriticalDirectiveFinder Visitor;
  Visitor.TraverseStmt(
      const_cast<OMPCriticalDirective *>(OuterCritical)->getAssociatedStmt());

  if (Visitor.InnerCriticalDirectives.empty())
    return;

  diag(OuterCritical->getBeginLoc(),
       "avoid nesting critical directives inside %0")
      << OuterCritical->getSourceRange()
      << OuterCritical->getDirectiveName().getName();

  for (const auto *const InnerCritical : Visitor.InnerCriticalDirectives) {
    diag(InnerCritical->getBeginLoc(),
         "inner critical directive %0 declared here",
         DiagnosticIDs::Level::Note)
        << InnerCritical->getSourceRange()
        << InnerCritical->getDirectiveName().getName();
  }
}

} // namespace clang::tidy::openmp
