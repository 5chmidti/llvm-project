// RUN: %check_clang_tidy %s openmp-overlapping-dependency-storage %t -- -- -fopenmp

void test0() {
    int arr[10];
    #pragma omp task depend(out: arr[0:10])
    ;
    #pragma omp task depend(out: arr[1:9])
    ;
// CHECK-MESSAGES: :[[@LINE-4]]:34: warning: the array sections 'arr[0:10]' and 'arr[1:9]' have overlapping storage [openmp-overlapping-dependency-storage]
}

void test1() {
    int arr[10];
    #pragma omp task depend(out: arr[0:10])
    ;
    #pragma omp task depend(out: arr)
    ;
}

void test2() {
    int arr[10];
    #pragma omp task depend(out: arr[0:5])
    ;
    #pragma omp task depend(out: arr)
    ;
// CHECK-MESSAGES: :[[@LINE-4]]:34: warning: the array sections 'arr[0:5]' and 'arr' have overlapping storage [openmp-overlapping-dependency-storage]
}

void test3() {
    int arr[10];
    #pragma omp task depend(out: arr[0:2])
    ;
    #pragma omp task depend(out: arr[4:2])
    ;
}

void test4() {
    int arr[10];
    #pragma omp task depend(out: arr[0:4])
    ;
    #pragma omp task depend(out: arr[2:4])
    ;
// CHECK-MESSAGES: :[[@LINE-4]]:34: warning: the array sections 'arr[0:4]' and 'arr[2:4]' have overlapping storage [openmp-overlapping-dependency-storage]
}

void test5() {
    int arr[10];
    #pragma omp task depend(out: arr[2:4])
    ;
    #pragma omp task depend(out: arr[2:4])
    ;
}

void test6(int arr[]) {
    #pragma omp task depend(out: arr[0:4])
    ;
    #pragma omp task depend(out: arr[2:4])
    ;
// CHECK-MESSAGES: :[[@LINE-4]]:34: warning: the array sections 'arr[0:4]' and 'arr[2:4]' have overlapping storage [openmp-overlapping-dependency-storage]
}

void test7(int arr[]) {
    #pragma omp task depend(out: arr[0:4])
    ;
    #pragma omp task depend(out: arr)
    ;
// CHECK-MESSAGES: :[[@LINE-4]]:34: warning: the array sections 'arr[0:4]' and 'arr' have overlapping storage [openmp-overlapping-dependency-storage]
}

void test8(int arr[]) {
    #pragma omp task depend(out: arr[1:4])
    ;
    #pragma omp task depend(out: arr[1:4])
    ;
}

void test9(int arr[]) {
    #pragma omp task depend(out: arr[0:4])
    {
        #pragma omp task depend(out: arr[2:4])
        ;
    }
}

void test10() {
    int arr[10];
    #pragma omp task depend(out: arr[0:10])
    ;
    #pragma omp task depend(out: arr[2])
    ;
// CHECK-MESSAGES: :[[@LINE-4]]:34: warning: the array sections 'arr[0:10]' and 'arr[2]' have overlapping storage [openmp-overlapping-dependency-storage]
}
