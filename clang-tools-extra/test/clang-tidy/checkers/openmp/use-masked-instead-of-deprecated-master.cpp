// RUN: %check_clang_tidy %s openmp-use-masked-instead-of-deprecated-master %t -- -- -fopenmp -fopenmp-version=52

void f() {
// CHECK-MESSAGES: :[[@LINE+3]]:5: warning: the 'master' directive is deprecated since OpenMP 5.2; use the 'masked' directive instead [openmp-use-masked-instead-of-deprecated-master]
// CHECK-FIXES: #pragma omp masked
// CHECK-FIXES-NEXT: ;
    #pragma omp master
        ;
}
