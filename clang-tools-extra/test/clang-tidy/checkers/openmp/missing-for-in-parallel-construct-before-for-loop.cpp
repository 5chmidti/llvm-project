// RUN: %check_clang_tidy %s openmp-missing-for-in-parallel-directive-before-for-loop %t -- --extra-arg=-fopenmp

void foo1(int A) {
    // CHECK-MESSAGES:[[@LINE+3]]:5: warning: OpenMP `parallel` directive does not contain work-sharing construct `for`, but for loop is the next statement; add `for` to the directive [openmp-missing-for-in-parallel-directive-before-for-loop]
    // CHECK-MESSAGES:[[@LINE+2]]:5: note: otherwise add a compound statement around the for loop to signal that this pattern is intended
    // CHECK-FIXES: #pragma omp parallel for firstprivate(A)
    #pragma omp parallel firstprivate(A)
    for (int I = 0; I < A; ++I) {}
}

void foo2(int B) {
    #pragma omp parallel for firstprivate(B)
    for (int I = 0; I < B; ++I) {}
}

void foo3(int C) {
    #pragma omp for
    for (int I = 0; I < C; ++I) {}
}
