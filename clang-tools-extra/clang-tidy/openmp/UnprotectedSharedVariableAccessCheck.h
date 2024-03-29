//===--- UnprotectedSharedVariableAccessCheck.h - clang-tidy ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_UNPROTECTEDSHAREDVARIABLEACCESSCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_UNPROTECTEDSHAREDVARIABLEACCESSCHECK_H

#include "../ClangTidyCheck.h"
#include "clang/AST/Decl.h"
#include "llvm/ADT/StringRef.h"
#include <vector>

namespace clang::tidy::openmp {

/// Finds unprotected accesses to shared variables.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/openmp/unprotected-shared-variable-access.html
class UnprotectedSharedVariableAccessCheck : public ClangTidyCheck {
public:
  UnprotectedSharedVariableAccessCheck(StringRef Name,
                                       ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.OpenMP;
  }
  std::optional<TraversalKind> getCheckTraversalKind() const override {
    return TK_IgnoreUnlessSpelledInSource;
  }

private:
  void
  checkSharedVariable(const ValueDecl *const SharedVar, const Stmt *const Scope,
                      ASTContext &Ctx,
                      const llvm::SmallVector<const ValueDecl *, 4> &LoopVars);

  std::vector<StringRef> ThreadSafeTypes;
  std::vector<StringRef> ThreadSafeFunctions;
};

} // namespace clang::tidy::openmp

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_UNPROTECTEDSHAREDVARIABLEACCESSCHECK_H
