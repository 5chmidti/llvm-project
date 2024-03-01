// RUN: %check_clang_tidy %s openmp-specify-schedule %t -- --extra-arg=-fopenmp

int ForWithoutScheduleInsideParallel(int*Buffer, int N){
    int Sum = 0;
    #pragma omp parallel default(none) firstprivate(N) shared(Buffer, Sum)
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the schedule for this openmp 'for' directive to fit the work distribution across iterations, the default is implementation defined [openmp-specify-schedule]
    #pragma omp for reduction(+:Sum)
    for (int I = 0; I < N; ++I) {
        Sum += Buffer[I];
    }
    return Sum;
}

int ForWithScheduleInsideParallel(int*Buffer, int N){
    int Sum = 0;
    #pragma omp parallel firstprivate(N) shared(Buffer, Sum)
    #pragma omp for schedule(static) reduction(+:Sum)
    for (int I = 0; I < N; ++I) {
        Sum += Buffer[I];
    }
    return Sum;
}

int ParallelForWithoutSchedule(int*Buffer, int N){
    int Sum = 0;
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the schedule for this openmp 'parallel for' directive to fit the work distribution across iterations, the default is implementation defined [openmp-specify-schedule]
    #pragma omp parallel for default(none) firstprivate(N) shared(Buffer) reduction(+:Sum)
    for (int I = 0; I < N; ++I) {
        Sum += Buffer[I];
    }
    return Sum;
}

int ParallelFor(int*Buffer, int N){
    int Sum = 0;
    #pragma omp parallel for schedule(static) default(none) firstprivate(N) shared(Buffer) reduction(+:Sum)
    for (int I = 0; I < N; ++I) {
        Sum += Buffer[I];
    }
    return Sum;
}

int ParallelForSimdWithoutSchedule(int*Buffer, int N){
    int Sum = 0;
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the schedule for this openmp 'parallel for simd' directive to fit the work distribution across iterations, the default is implementation defined [openmp-specify-schedule]
    #pragma omp parallel for simd default(none) firstprivate(N) shared(Buffer) reduction(+:Sum)
    for (int I = 0; I < N; ++I) {
        Sum += Buffer[I];
    }
    return Sum;
}

int ParallelForSimdWithSchedule(int*Buffer, int N){
    int Sum = 0;
    #pragma omp parallel for simd schedule(static) default(none) firstprivate(N) shared(Buffer) reduction(+:Sum)
    for (int I = 0; I < N; ++I) {
        Sum += Buffer[I];
    }
    return Sum;
}

void Parallel(){
    #pragma omp parallel
        { }
}
