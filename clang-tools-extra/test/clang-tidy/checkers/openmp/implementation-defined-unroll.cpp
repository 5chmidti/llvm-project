// RUN: %check_clang_tidy %s openmp-implementation-defined-unroll %t -- -- -fopenmp


void f() {
    #pragma omp unroll
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: not specifying a 'partial' or 'full' clause will result in implementation defined behavior for the unrolling [openmp-implementation-defined-unroll]
    for (int i = 0; i < 10; ++i) {}
    #pragma omp unroll full
    for (int i = 0; i < 10; ++i) {}
    #pragma omp unroll partial
// CHECK-MESSAGES: :[[@LINE-1]]:24: warning: the implicit unroll factor of the 'partial' unroll clause is implementation defined [openmp-implementation-defined-unroll]
    for (int i = 0; i < 10; ++i) {}
    #pragma omp unroll partial(8)
    for (int i = 0; i < 10; ++i) {}
}
