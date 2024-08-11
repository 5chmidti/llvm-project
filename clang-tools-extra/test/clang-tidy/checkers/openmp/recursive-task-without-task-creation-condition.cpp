// RUN: %check_clang_tidy %s openmp-recursive-task-without-task-creation-condition %t -- -- -fopenmp

int recursive(int v) {
    if (v == 1) return 1;
    int result = v;
    #pragma omp task depend(inout: result) depend(in: v)
        result *= recursive(v-1);
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: recursive task creation without an 'if' or 'final' clause may degrade performance due to the task-management overhead [openmp-recursive-task-without-task-creation-condition]
// CHECK-MESSAGES: :[[@LINE-3]]:5: note: the 'depend' clause of a task is ignored when the 'if' or 'final' clause evalutates to 'false'; ensure correctness if required
// CHECK-MESSAGES: :[[@LINE-3]]:19: note: task reached by calling 'recursive' here
    #pragma omp task depend(inout: result) depend(in: v)
        result *= recursive(v-1);
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: recursive task creation without an 'if' or 'final' clause may degrade performance due to the task-management overhead [openmp-recursive-task-without-task-creation-condition]
// CHECK-MESSAGES: :[[@LINE-3]]:5: note: the 'depend' clause of a task is ignored when the 'if' or 'final' clause evalutates to 'false'; ensure correctness if required
// CHECK-MESSAGES: :[[@LINE-3]]:19: note: task reached by calling 'recursive' here
    return result;
}
int recursiveWithIf(int v) {
    if (v == 1) return 1;
    int result = v;
    #pragma omp task if(v > 100)
        result *= recursive(v-1);
    return result;
}

int forwardWithStep(int v);
int forwardWithStepWithIf(int v);
int recursiveWithStep(int v) {
    if (v == 1) return 1;
    int result = v;
    #pragma omp task
        result *= forwardWithStep(v-1);
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: recursive task creation without an 'if' or 'final' clause may degrade performance due to the task-management overhead [openmp-recursive-task-without-task-creation-condition]
// CHECK-MESSAGES: :[[@LINE-2]]:19: note: task reached by calling 'forwardWithStep' here
// CHECK-MESSAGES: :[[@LINE+10]]:37: note: task reached by calling 'recursiveWithStep' here
    return result;
}
int recursiveWithStepWithIf(int v) {
    if (v == 1) return 1;
    int result = v;
    #pragma omp task if(v > 100)
        result *= forwardWithStepWithIf(v-1);
    return result;
}
int forwardWithStep(int v) { return recursiveWithStep(v); }
int forwardWithStepWithIf(int v) { return recursiveWithStepWithIf(v); }

int recursive_withFinal(int v) {
    if (v == 1) return 1;
    int result = v;
    #pragma omp task depend(inout: result) depend(in: v) final(v < 10)
        result *= recursive(v-1);
    #pragma omp task depend(inout: result) depend(in: v) final(v < 10)
        result *= recursive(v-1);
    return result;
}
