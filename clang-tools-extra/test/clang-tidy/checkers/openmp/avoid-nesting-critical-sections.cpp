// RUN: %check_clang_tidy %s openmp-avoid-nesting-critical-sections %t -- --extra-arg=-fopenmp

void ImmediateCritialSectionNesting(bool* flags, int N) {
  int x = 0;
  int y = 0;
  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside 'X' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :[[@LINE+2]]:9: note: inner critical directive 'Y' declared here
      #pragma omp critical(X)
        #pragma omp critical(Y)
          x = y;
    else
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside 'Y' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :[[@LINE+2]]:9: note: inner critical directive 'X' declared here
      #pragma omp critical(Y)
        #pragma omp critical(X)
          x = y;
  }
}

void CriticalSectionNestingWithCompoundStmts(bool* flags, int N) {
  int x = 0;
  int y = 0;
  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside '' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :[[@LINE+3]]:9: note: inner critical directive 'Y' declared here
      #pragma omp critical
      {
        #pragma omp critical(Y)
          x = y;
      }
    else
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside 'Y' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :[[@LINE+3]]:9: note: inner critical directive '' declared here
      #pragma omp critical(Y)
      {
        #pragma omp critical
          x = y;
      }
  }
}

void foo();
void fooWithVisibleCritical() {
    #pragma omp critical
        int x;
}

void CriticalSectionNestingWithVisibleFunctionBody(bool* flags, int N) {
  int x = 0;
  int y = 0;
  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside 'X' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :49:5: note: inner critical directive '' declared here
      #pragma omp critical(X)
        fooWithVisibleCritical();
    else
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside 'Y' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :49:5: note: inner critical directive '' declared here
      #pragma omp critical(Y)
        fooWithVisibleCritical();
  }
}

void CriticalSectionNestingWithLambdaBody(bool* flags, int N) {
  int x = 0;
  int y = 0;

  const auto CriticalLambda = [](){
    #pragma omp critical
      int x;
  };

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside 'X' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :[[@LINE-8]]:5: note: inner critical directive '' declared here
      #pragma omp critical(X)
        CriticalLambda();
    else
// CHECK-MESSAGES: :[[@LINE+2]]:7: warning: avoid nesting critical directives inside 'Y' [openmp-avoid-nesting-critical-sections]
// CHECK-MESSAGES: :[[@LINE-13]]:5: note: inner critical directive '' declared here
      #pragma omp critical(Y)
        CriticalLambda();
  }
}

void CriticalSectionNestingWithoutVisibleFunctionBody(bool* flags, int N) {
  int x = 0;
  int y = 0;
  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
      #pragma omp critical(X)
        foo();
    else
      #pragma omp critical(Y)
        foo();
  }
}
