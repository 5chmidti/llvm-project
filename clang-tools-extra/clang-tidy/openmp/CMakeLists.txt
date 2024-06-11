set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyOpenMPModule
  AvoidNestingCriticalSectionsCheck.cpp
  CriticalSectionDeadlockCheck.cpp
  DeclareLoopIterationVariableInForInitStatementCheck.cpp
  DoNotModifyLoopVariableCheck.cpp
  ExceptionEscapeCheck.cpp
  ImplementationDefinedUnrollCheck.cpp
  MissingForInParallelDirectiveBeforeForLoopCheck.cpp
  MissingOrderedDirectiveAfterOrderedClauseCheck.cpp
  OpenMPTidyModule.cpp
  RecursiveTaskWithoutTaskCreationConditionCheck.cpp
  TaskDependenciesCheck.cpp
  UseAtomicInsteadOfCriticalCheck.cpp
  SpecifyScheduleCheck.cpp
  UnprotectedSharedVariableAccessCheck.cpp
  UseDefaultNoneCheck.cpp
  UseMaskedInsteadOfDeprecatedMasterCheck.cpp
  UseReductionCheck.cpp

  LINK_LIBS
  clangTidy
  clangTidyUtils

  DEPENDS
  omp_gen
  )

clang_target_link_libraries(clangTidyOpenMPModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  )
