// RUN: %check_clang_tidy %s openmp-use-reduction %t -- --extra-arg=-fopenmp

void test() {
    int Sum = 0;
    #pragma omp parallel for
    for (int i = 0; i< 10; ++i)
        #pragma omp atomic
            Sum += i;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'f' is insufficiently awesome [openmp-use-reduction]

    #pragma omp parallel
    #pragma omp for
    for (int i = 0; i< 10; ++i)
        #pragma omp atomic
            Sum = i + Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'f' is insufficiently awesome [openmp-use-reduction]

    #pragma omp parallel
    {
        int Local = 0;
        #pragma omp for
        for (int i = 0; i< 10; ++i)
            Local = i + Local;
    }
}
