// RUN: %check_clang_tidy %s openmp-use-reduction %t -- --extra-arg=-fopenmp

void test() {
    int Sum = 0;
    #pragma omp parallel for
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: prefer to use a reduction [openmp-use-reduction]
    for (int i = 0; i< 10; ++i)
        #pragma omp atomic
            Sum += i;

    #pragma omp parallel
    #pragma omp for
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: prefer to use a reduction [openmp-use-reduction]
    for (int i = 0; i< 10; ++i)
        #pragma omp atomic
            Sum = i + Sum;

    #pragma omp parallel
    {
        int Local = 0;
        #pragma omp for
        for (int i = 0; i< 10; ++i)
            Local = i + Local;
    }
}
