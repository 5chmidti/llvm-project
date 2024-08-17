// RUN: %check_clang_tidy %s openmp-use-atomic-instead-of-critical %t -- -- -fopenmp

int f();

void foo() {
    int x = 10;
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-use-atomic-instead-of-critical]
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

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-use-atomic-instead-of-critical]
    #pragma omp parallel default(shared)
    {
// CHECK-FIXES: #pragma omp atomic write
// CHECK-FIXES-NEXT: x = 1;
        #pragma omp critical
            x = 1;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
        #pragma omp critical
            ++x;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
         #pragma omp critical
            { ++x; }
    }

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-use-atomic-instead-of-critical]
    #pragma omp parallel
    {
// CHECK-FIXES: #pragma omp atomic write
// CHECK-FIXES-NEXT: x = 3;
        #pragma omp critical
            x = 3;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
        #pragma omp critical
            ++x;

// CHECK-FIXES: #pragma omp atomic update
// CHECK-FIXES-NEXT: ++x;
         #pragma omp critical
            { ++x; }
    }

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-use-atomic-instead-of-critical]
    #pragma omp parallel
    {
// CHECK-FIXES: #pragma omp atomic write
// CHECK-FIXES-NEXT: x = 3 + 2 + 1;
        #pragma omp critical
            x = 3 + 2 + 1;
    }

    #pragma omp parallel
    {
        #pragma omp critical
            x = x + x + x;
    }

// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the accesses to 'x' inside critical sections can be rewritten to use 'omp atomic's for more performance [openmp-use-atomic-instead-of-critical]
    #pragma omp parallel
    {
// CHECK-FIXES: #pragma omp atomic write
// CHECK-FIXES-NEXT: x = f();
        #pragma omp critical
            x = f();
    }

    #pragma omp parallel default(shared) private(x)
    {
// CHECK-MESSAGES: :[[@LINE+2]]:13: warning: this operation is declared with 'omp atomic' but it does not involve a shared variable [openmp-use-atomic-instead-of-critical]
        #pragma omp atomic write
            x = 2;

// CHECK-MESSAGES: :[[@LINE+2]]:13: warning: this operation is declared with 'omp atomic' but it does not involve a shared variable [openmp-use-atomic-instead-of-critical]
        #pragma omp atomic update
            ++x;
    }

    #pragma omp atomic update
        ++x;

}
