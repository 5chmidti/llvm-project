// RUN: %check_clang_tidy %s openmp-do-not-set-num-threads-in-parallel-region %t -- -- -fopenmp

void omp_set_num_threads(int num_threads);
void nested_set_num_threads(int num_threads) { omp_set_num_threads(num_threads); }

void test() {
    omp_set_num_threads(42);

    #pragma omp parallel
    {
        omp_set_num_threads(1);
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not call 'omp_set_num_threads' inside this parallel region, it will only affect newly entered parallel regions [openmp-do-not-set-num-threads-in-parallel-region]
    }

    #pragma omp parallel
    {
        omp_set_num_threads(1);
        #pragma omp parallel
        ;
        #pragma omp parallel
        ;
    }

    #pragma omp parallel
    {
        nested_set_num_threads(2);
    }

    #pragma omp parallel
    {
        nested_set_num_threads(3);
        #pragma omp parallel
        ;
        #pragma omp parallel
        ;
    }
}
