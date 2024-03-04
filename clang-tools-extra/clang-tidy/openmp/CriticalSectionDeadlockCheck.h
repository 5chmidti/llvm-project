//===--- CriticalSectionDeadlockCheck.h - clang-tidy ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_CRITICALSECTIONDEADLOCKCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_CRITICALSECTIONDEADLOCKCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::openmp {

/// Detects deadlocks from nesting OpenMP critical regions.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/openmp/critical-section-deadlock.html
class CriticalSectionDeadlockCheck : public ClangTidyCheck {
public:
  CriticalSectionDeadlockCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;

private:
  const bool IgnoreBarriers;
};

} // namespace clang::tidy::openmp

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_OPENMP_CRITICALSECTIONDEADLOCKCHECK_H
