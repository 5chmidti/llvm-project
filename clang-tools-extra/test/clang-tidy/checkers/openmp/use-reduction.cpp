// RUN: %check_clang_tidy %s openmp-use-reduction %t -- --extra-arg=-fopenmp

void sink(int);

void test() {
    int Sum = 0;
    #pragma omp parallel for
    for (int i = 0; i < 10; ++i)
        #pragma omp atomic
            Sum += i;
// CHECK-MESSAGES: :[[@LINE-4]]:5: warning: prefer to use a 'reduction(+ : Sum)' clause instead of explicit synchronization for each update [openmp-use-reduction]
// CHECK-FIXES: #pragma omp parallel for reduction(+ : Sum)
// CHECK-FIXES-NEXT: for (int i = 0; i < 10; ++i)
// CHECK-FIXES-NEXT: {{^\ +}}{{$}}
// CHECK-FIXES-NEXT: Sum += i;

    #pragma omp parallel
    #pragma omp for
    for (int i = 0; i < 10; ++i)
        #pragma omp atomic
            Sum = i + Sum;
// CHECK-MESSAGES: :[[@LINE-4]]:5: warning: prefer to use a 'reduction(+ : Sum)' clause instead of explicit synchronization for each update [openmp-use-reduction]
// CHECK-FIXES: #pragma omp parallel
// CHECK-FIXES: #pragma omp for reduction(+ : Sum)
// CHECK-FIXES-NEXT: for (int i = 0; i < 10; ++i)
// CHECK-FIXES-NEXT: {{^\ +}}{{$}}
// CHECK-FIXES-NEXT: Sum = i + Sum;

    #pragma omp parallel for
    for (int i = 0; i < 10; ++i)
        #pragma omp critical
            Sum += i;
// CHECK-MESSAGES: :[[@LINE-4]]:5: warning: prefer to use a 'reduction(+ : Sum)' clause instead of explicit synchronization for each update [openmp-use-reduction]

    #pragma omp parallel
    #pragma omp for
    for (int i = 0; i < 10; ++i)
        #pragma omp critical
            Sum = i + Sum;
// CHECK-MESSAGES: :[[@LINE-4]]:5: warning: prefer to use a 'reduction(+ : Sum)' clause instead of explicit synchronization for each update [openmp-use-reduction]

    #pragma omp parallel
    #pragma omp for
    for (int i = 0; i < 10; ++i)
        #pragma omp critical
            Sum = i + Sum + i + i + i;

    #pragma omp parallel
    #pragma omp for
    for (int i = 0; i < 10; ++i) {
        #pragma omp critical
            sink(Sum);
        #pragma omp critical
            Sum = i + Sum;
    }

    #pragma omp parallel
    {
        int Local = 0;
        #pragma omp for
        for (int i = 0; i < 10; ++i)
            Local = i + Local;
    }
}
