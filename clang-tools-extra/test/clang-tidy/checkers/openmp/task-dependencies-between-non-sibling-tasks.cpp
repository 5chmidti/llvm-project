// RUN: %check_clang_tidy %s openmp-task-dependencies-between-non-sibling-tasks %t -- -- -fopenmp

void f() {
    int value = 0;
    int other = 0;
    #pragma omp task default(none) shared(value) depend(out: value)
    {
// CHECK-MESSAGES: :[[@LINE+2]]:65: warning: the dependency of 'value' from a child task will not impose ordering with the dependency of its ancestors dependency [openmp-task-dependencies-between-non-sibling-tasks]
// CHECK-MESSAGES: :[[@LINE-3]]:62: note: 'value' was specified in a 'depend' clause here
        #pragma omp task default(none) shared(value) depend(in: value)
            int copy = value;
        ++value;
    }

    #pragma omp task default(none) shared(value, other) depend(out: value, other)
    {
        ++value;
        ++other;
// CHECK-MESSAGES: :[[@LINE+4]]:72: warning: the dependency of 'value' from a child task will not impose ordering with the dependency of its ancestors dependency [openmp-task-dependencies-between-non-sibling-tasks]
// CHECK-MESSAGES: :[[@LINE-5]]:69: note: 'value' was specified in a 'depend' clause here
// CHECK-MESSAGES: :[[@LINE+2]]:79: warning: the dependency of 'other' from a child task will not impose ordering with the dependency of its ancestors dependency [openmp-task-dependencies-between-non-sibling-tasks]
// CHECK-MESSAGES: :[[@LINE-7]]:76: note: 'other' was specified in a 'depend' clause here
        #pragma omp task default(none) shared(value, other) depend(in: value, other)
        {
            int copy = value;
            int otherCopy = other;
        }
        ++value;
    }
}
