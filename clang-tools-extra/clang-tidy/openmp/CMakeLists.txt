set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyOpenMPModule
  DeclareLoopIterationVariableInForInitStatementCheck.cpp
  DoNotModifyLoopVariableCheck.cpp
  ExceptionEscapeCheck.cpp
  MissingForInParallelDirectiveBeforeForLoopCheck.cpp
  OpenMPTidyModule.cpp
  UnprotectedSharedVariableAccessCheck.cpp
  UseDefaultNoneCheck.cpp

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
