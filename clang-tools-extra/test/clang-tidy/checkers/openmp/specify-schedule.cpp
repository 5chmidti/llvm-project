// RUN: %check_clang_tidy %s openmp-specify-schedule %t -- --extra-arg=-fopenmp

int ForWithoutScheduleInsideParallel(int*Buffer, int N){
    int Sum = 0;
    #pragma omp parallel default(none) firstprivate(N) shared(Buffer, Sum)
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the schedule for this OpenMP 'for' directive to fit the work distribution across iterations, the default is implementation defined [openmp-specify-schedule]
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
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the schedule for this OpenMP 'parallel for' directive to fit the work distribution across iterations, the default is implementation defined [openmp-specify-schedule]
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
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the schedule for this OpenMP 'parallel for simd' directive to fit the work distribution across iterations, the default is implementation defined [openmp-specify-schedule]
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

int ParallelForWithAutoSchedyle(int*Buffer, int N){
    int Sum = 0;
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: the 'auto' scheduling kind results in an implementation defined schedule, which may not fit the work distribution across iterations [openmp-specify-schedule]
    #pragma omp parallel for simd schedule(auto) default(none) firstprivate(N) shared(Buffer) reduction(+:Sum)
    for (int I = 0; I < N; ++I) {
        Sum += Buffer[I];
    }
    return Sum;
}

int OrderedNoSchedule(int*Buffer, int N){
    int Sum = 0;
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the schedule for this OpenMP 'parallel for' directive to fit the ordering dependencies and work distribution across iterations, the default is implementation defined [openmp-specify-schedule]
    #pragma omp parallel for ordered
    for (int I = 0; I < N; ++I) {
        #pragma omp ordered
        Sum += Buffer[I];
    }
    return Sum;
}

int OrderedStaticSchedule(int*Buffer, int N){
    int Sum = 0;
// CHECK-MESSAGES: :[[@LINE+1]]:5: warning: specify the chunk-size for the 'static' schedule to avoid executing chunks that are too large which may lead to serialized execution due to the 'ordered' clause [openmp-specify-schedule]
    #pragma omp parallel for ordered schedule(static)
    for (int I = 0; I < N; ++I) {
        #pragma omp ordered
        Sum += Buffer[I];
    }
    return Sum;
}

int OrderedStaticScheduleWithChunkSize(int*Buffer, int N){
    int Sum = 0;
    #pragma omp parallel for ordered schedule(static, 16)
    for (int I = 0; I < N; ++I) {
        #pragma omp ordered
        Sum += Buffer[I];
    }
    return Sum;
}

int OrderedDynamicchedule(int*Buffer, int N){
    int Sum = 0;
    #pragma omp parallel for ordered schedule(dynamic)
    for (int I = 0; I < N; ++I) {
        #pragma omp ordered
        Sum += Buffer[I];
    }
    return Sum;
}
