//===--- DoNotModifyLoopVariableCheck.h - clang-tidy ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_DONOTMODIFYLOOPVARIABLECHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_DONOTMODIFYLOOPVARIABLECHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::openmp {

/// Finds openmp loop directives where the loop counter or condition is
/// modified in the body.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/openmp/do-not-modify-loop-variable.html
class DoNotModifyLoopVariableCheck : public ClangTidyCheck {
public:
  DoNotModifyLoopVariableCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  bool isLanguageVersionSupported(const LangOptions &LangOpts) const override {
    return LangOpts.OpenMP;
  }
  std::optional<TraversalKind> getCheckTraversalKind() const override {
    return TK_IgnoreUnlessSpelledInSource;
  }
};

} // namespace clang::tidy::openmp

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_DONOTMODIFYLOOPVARIABLECHECK_H
