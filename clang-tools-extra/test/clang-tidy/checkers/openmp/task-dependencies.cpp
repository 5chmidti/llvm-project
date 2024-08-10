// RUN: %check_clang_tidy %s openmp-task-dependencies %t -- -- -fopenmp

void f() {
    int Var = 0;
    int Array[100];

    #pragma omp task
    {
        Var = 0;
        ++Array[0];
    }

    #pragma omp task depend(out: Var)
    {
        Var = 0;
    }

    #pragma omp task depend(out: Var)
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: the dependency of 'Var' is under-specified; use 'inout' [openmp-task-dependencies]
    {
        ++Var;
    }

    #pragma omp task depend(out: Var)
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: the dependency of 'Var' should be 'in' [openmp-task-dependencies]
    {
        int Other = Var;
    }

    #pragma omp task depend(inout: Var)
// CHECK-MESSAGES: :[[@LINE-1]]:36: warning: the dependency of 'Var' is over-specified; use 'out' [openmp-task-dependencies]
    {
        Var = 0;
    }

    #pragma omp task depend(inout: Var)
    {
        ++Var;
    }

    #pragma omp task depend(inout: Var)
// CHECK-MESSAGES: :[[@LINE-1]]:36: warning: the dependency of 'Var' is over-specified; use 'in' [openmp-task-dependencies]
    {
        int Other = Var;
    }

    #pragma omp task depend(in: Var)
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: the dependency of 'Var' should be 'out' [openmp-task-dependencies]
    {
        Var = 0;
    }

    #pragma omp task depend(in: Var)
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: the dependency of 'Var' is under-specified; use 'inout' [openmp-task-dependencies]
    {
        ++Var;
    }

    #pragma omp task depend(in: Var)
    {
        int Other = Var;
    }

    #pragma omp task depend(out: Array[0])
    {
        Array[0] = 0;
    }

    #pragma omp task depend(out: Array[1])
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: the dependency of 'Array[1]' is under-specified; use 'inout' [openmp-task-dependencies]
    {
        ++Array[1];
    }

    #pragma omp task depend(out: Array[2])
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: the dependency of 'Array[2]' should be 'in' [openmp-task-dependencies]
    {
        int Other = Array[2];
    }

    #pragma omp task depend(inout: Array[3])
// CHECK-MESSAGES: :[[@LINE-1]]:36: warning: the dependency of 'Array[3]' is over-specified; use 'out' [openmp-task-dependencies]
    {
        Array[3] = 0;
    }

    #pragma omp task depend(inout: Array[4])
    {
        ++Array[4];
    }

    #pragma omp task depend(inout: Array[5])
// CHECK-MESSAGES: :[[@LINE-1]]:36: warning: the dependency of 'Array[5]' is over-specified; use 'in' [openmp-task-dependencies]
    {
        int Other = Array[5];
    }

    #pragma omp task depend(in: Array[6])
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: the dependency of 'Array[6]' should be 'out' [openmp-task-dependencies]
    {
        Array[6] = 0;
    }

    #pragma omp task depend(in: Array[7])
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: the dependency of 'Array[7]' is under-specified; use 'inout' [openmp-task-dependencies]
    {
        ++Array[7];
    }

    #pragma omp task depend(in: Array[8])
    {
        int Other = Array[8];
    }

    #pragma omp task depend(out: Array[0:100])
    {
        Array[9] = 0;
    }

    #pragma omp task depend(out: Array[0:100])
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: the dependency of 'Array[0:100]' is under-specified; use 'inout' [openmp-task-dependencies]
    {
        ++Array[10];
    }

    #pragma omp task depend(out: Array[0:100])
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: the dependency of 'Array[0:100]' should be 'in' [openmp-task-dependencies]
    {
        int Other = Array[11];
    }

    #pragma omp task depend(inout: Array[0:100])
// CHECK-MESSAGES: :[[@LINE-1]]:36: warning: the dependency of 'Array[0:100]' is over-specified; use 'out' [openmp-task-dependencies]
    {
        Array[12] = 0;
    }

    #pragma omp task depend(inout: Array[0:100])
    {
        ++Array[13];
    }

    #pragma omp task depend(inout: Array[0:100])
// CHECK-MESSAGES: :[[@LINE-1]]:36: warning: the dependency of 'Array[0:100]' is over-specified; use 'in' [openmp-task-dependencies]
    {
        int Other = Array[14];
    }

    #pragma omp task depend(in: Array[0:100])
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: the dependency of 'Array[0:100]' should be 'out' [openmp-task-dependencies]
    {
        Array[15] = 0;
    }

    #pragma omp task depend(in: Array[0:100])
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: the dependency of 'Array[0:100]' is under-specified; use 'inout' [openmp-task-dependencies]
    {
        ++Array[16];
    }

    #pragma omp task depend(in: Array[0:100])
    {
        int Other = Array[17];
    }

    int* ptr = nullptr;
    #pragma omp task depend(in: ptr)
// CHECK-MESSAGES: :[[@LINE-1]]:33: warning: depending on pointer 'ptr' will create a dependency on the storage location of the pointer, not the underlying memory [openmp-task-dependencies]
        int Other = *ptr;
}

// contains 'dependencies' to non-sibling tasks, which are wrong of course
void nested() {
    int Var = 0;

    #pragma omp task depend(out: Var)
    {
        Var = 10;
        #pragma omp task depend(in: Var)
        int Other = Var;
    }

    #pragma omp task depend(out: Var)
    {
        Var = 10;
        #pragma omp task depend(inout: Var)
        ++Var;
    }

    #pragma omp task depend(out: Var)
    {
        Var = 10;
        #pragma omp task depend(out: Var)
        Var = 10;
    }
}

