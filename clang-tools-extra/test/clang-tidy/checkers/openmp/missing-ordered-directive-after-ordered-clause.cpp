// RUN: %check_clang_tidy %s openmp-missing-ordered-directive-after-ordered-clause %t -- -- -fopenmp


void f() {
    #pragma omp parallel for ordered
    for (int i = 0; i < 10; ++i) {
        #pragma omp ordered
            ;
    }

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: this 'parallel for' directive has an 'ordered' clause but the associated statement has no 'ordered' directive [openmp-missing-ordered-directive-after-ordered-clause]
    #pragma omp parallel for ordered
    for (int i = 0; i < 10; ++i) {
            ;
    }

    #pragma omp parallel for ordered(2)
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
        #pragma omp ordered depend(sink:i-1,j-1)
            ;
        #pragma omp ordered depend(source)
        }
    }

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: this 'parallel for' directive has an 'ordered' clause but the associated statement has no 'ordered' directive [openmp-missing-ordered-directive-after-ordered-clause]
    #pragma omp parallel for ordered(2)
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            ;
        }
    }
}
