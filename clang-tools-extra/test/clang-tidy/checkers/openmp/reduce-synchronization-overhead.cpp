// RUN: %check_clang_tidy %s openmp-reduce-synchronization-overhead %t -- -- -fopenmp

void foo() {
    int x = 10;
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-reduce-synchronization-overhead]
    #pragma omp parallel default(none) shared(x)
    {
// CHECK-FIXES: #pragma omp atomic write
// CHECK-FIXES-NEXT: x = 0;
        #pragma omp critical
            x = 0;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
        #pragma omp critical
            ++x;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
         #pragma omp critical
            { ++x; }
    }

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-reduce-synchronization-overhead]
    #pragma omp parallel default(shared)
    {
// CHECK-FIXES: #pragma omp atomic write
// CHECK-FIXES-NEXT: x = 0;
        #pragma omp critical
            x = 0;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
        #pragma omp critical
            ++x;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
         #pragma omp critical
            { ++x; }
    }

    #pragma omp parallel default(shared) private(x)
    {
// CHECK-MESSAGES: :[[@LINE+4]]:13: warning: this operation is declared with `omp atomic` but it does not involve a shared variable [openmp-reduce-synchronization-overhead]
// CHECK-FIXES: {{^        $}}
// CHECK-FIXES-NEXT: x = 0;
        #pragma omp atomic write
            x = 0;

// CHECK-MESSAGES: :[[@LINE+4]]:13: warning: this operation is declared with `omp atomic` but it does not involve a shared variable [openmp-reduce-synchronization-overhead]
// CHECK-FIXES: {{^        $}}
// CHECK-FIXES-NEXT: ++x;
        #pragma omp atomic update
            ++x;
    }

    #pragma omp atomic update
        ++x;

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-reduce-synchronization-overhead]
    #pragma omp parallel
    {
// CHECK-FIXES: #pragma omp atomic write
// CHECK-FIXES-NEXT: x = 0;
        #pragma omp critical
            x = 0;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
        #pragma omp critical
            ++x;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
         #pragma omp critical
            { ++x; }
    }
}
