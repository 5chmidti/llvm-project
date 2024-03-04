//===--- OpenMPTidyModule.cpp - clang-tidy--------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"
#include "../ClangTidyModule.h"
#include "../ClangTidyModuleRegistry.h"
#include "AvoidNestingCriticalSectionsCheck.h"
#include "CriticalSectionDeadlockCheck.h"
#include "DeclareLoopIterationVariableInForInitStatementCheck.h"
#include "DoNotModifyLoopVariableCheck.h"
#include "ExceptionEscapeCheck.h"
#include "MissingForInParallelDirectiveBeforeForLoopCheck.h"
#include "SpecifyScheduleCheck.h"
#include "UnprotectedSharedVariableAccessCheck.h"
#include "UseDefaultNoneCheck.h"

namespace clang::tidy {
namespace openmp {

/// This module is for OpenMP-specific checks.
class OpenMPModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<AvoidNestingCriticalSectionsCheck>(
        "openmp-avoid-nesting-critical-sections");
    CheckFactories.registerCheck<CriticalSectionDeadlockCheck>(
        "openmp-critical-section-deadlock");
    CheckFactories
        .registerCheck<DeclareLoopIterationVariableInForInitStatementCheck>(
            "openmp-declare-loop-iteration-variable-in-for-init-statement");
    CheckFactories.registerCheck<DoNotModifyLoopVariableCheck>(
        "openmp-do-not-modify-loop-variable");
    CheckFactories.registerCheck<ExceptionEscapeCheck>(
        "openmp-exception-escape");
    CheckFactories
        .registerCheck<MissingForInParallelDirectiveBeforeForLoopCheck>(
            "openmp-missing-for-in-parallel-directive-before-for-loop");
    CheckFactories.registerCheck<SpecifyScheduleCheck>(
        "openmp-specify-schedule");
    CheckFactories.registerCheck<UnprotectedSharedVariableAccessCheck>(
        "openmp-unprotected-shared-variable-access");
    CheckFactories.registerCheck<UseDefaultNoneCheck>(
        "openmp-use-default-none");
  }
};

// Register the OpenMPTidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<OpenMPModule>
    X("openmp-module", "Adds OpenMP-specific checks.");

} // namespace openmp

// This anchor is used to force the linker to link in the generated object file
// and thus register the OpenMPModule.
volatile int OpenMPModuleAnchorSource = 0;

} // namespace clang::tidy
