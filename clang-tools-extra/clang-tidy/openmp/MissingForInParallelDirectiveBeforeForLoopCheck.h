//===--- MissingForInParallelDirectiveBeforeForLoopCheck.h - clang-tidy *- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_MISSINGFORINPARALLELCONSTRUCTBEFOREFORLOOPCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_MISSINGFORINPARALLELCONSTRUCTBEFOREFORLOOPCHECK_H

#include "../ClangTidyCheck.h"
#include <clang/AST/ASTTypeTraits.h>

namespace clang::tidy::openmp {

/// Finds OpenMP parallel directives where the next statement is a for loop,
/// and the ``parallel`` directive is not a ``parallel for`` directive.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/openmp/missing-for-in-parallel-directive-before-for-loop.html
class MissingForInParallelDirectiveBeforeForLoopCheck : public ClangTidyCheck {
public:
  MissingForInParallelDirectiveBeforeForLoopCheck(StringRef Name,
                                                  ClangTidyContext *Context)
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

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_MISSINGFORINPARALLELCONSTRUCTBEFOREFORLOOPCHECK_H
