// RUN: %check_clang_tidy %s openmp-do-not-modify-loop-variable %t -- --extra-arg=-fopenmp

void doesMutate(int&);
void doesNotMutate(const int&);

void test(int *Buffer, int BufferSize) {
    #pragma omp parallel for firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        if (Buffer[LoopVar] > 0) {
            ++LoopVar;
// CHECK-MESSAGES:[[@LINE-1]]:15: warning: do not mutate the variable 'LoopVar' used as the for statement counter of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
        }
    }

    #pragma omp parallel for firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        if (Buffer[LoopVar] > 0) {
            doesMutate(LoopVar);
// CHECK-MESSAGES:[[@LINE-1]]:24: warning: do not mutate the variable 'LoopVar' used as the for statement counter of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
            doesNotMutate(LoopVar);
        }
    }

    #pragma omp parallel for firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        if (Buffer[LoopVar] > 0) {
            BufferSize = 42;
// CHECK-MESSAGES:[[@LINE-1]]:13: warning: do not mutate the variable 'BufferSize' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-4]]:38: note: variable 'BufferSize' used in loop condition here
        }
    }

    #pragma omp parallel for firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        for (int LoopVar2 = 0; LoopVar2 < (LoopVar * LoopVar); ++LoopVar2) {
            if (Buffer[LoopVar] > 0) {
                BufferSize = 42;
// CHECK-MESSAGES:[[@LINE-1]]:17: warning: do not mutate the variable 'BufferSize' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-5]]:38: note: variable 'BufferSize' used in loop condition here
// CHECK-MESSAGES:[[@LINE-6]]:51: note: variable 'BufferSize' used in loop condition here
            }
        }
    }

    #pragma omp parallel for firstprivate(BufferSize) collapse(2)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        for (int LoopVar2 = 0; LoopVar2 < (BufferSize * BufferSize); ++LoopVar2) {
            if (Buffer[LoopVar] > 0) {
                BufferSize = 42;
// CHECK-MESSAGES:[[@LINE-1]]:17: warning: do not mutate the variable 'BufferSize' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-5]]:38: note: variable 'BufferSize' used in loop condition here
// CHECK-MESSAGES:[[@LINE-6]]:51: note: variable 'BufferSize' used in loop condition here
            }
        }
    }


    #pragma omp parallel for firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        for (int LoopVar2 = 0; LoopVar2 < (LoopVar * LoopVar); ++LoopVar2) {
            if (Buffer[LoopVar] > 0) {
                LoopVar = 42;
// CHECK-MESSAGES:[[@LINE-1]]:17: warning: do not mutate the variable 'LoopVar' used as the for statement counter of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
            }
        }
    }

    #pragma omp parallel for firstprivate(BufferSize) collapse(2)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        for (int LoopVar2 = 0; LoopVar2 < (BufferSize * BufferSize); ++LoopVar2) {
            if (Buffer[LoopVar] > 0) {
                LoopVar = 42;
// CHECK-MESSAGES:[[@LINE-1]]:17: warning: do not mutate the variable 'LoopVar' used as the for statement counter of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
            }
        }
    }

    #pragma omp parallel for firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
        if (Buffer[LoopVar] > 0) {
            doesMutate(BufferSize);
// CHECK-MESSAGES:[[@LINE-1]]:24: warning: do not mutate the variable 'BufferSize' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-4]]:38: note: variable 'BufferSize' used in loop condition here
            doesNotMutate(BufferSize);
        }
    }

    // make buffer[1] the loop cond
    int LoopVar = 0;
    #pragma omp parallel for
    for (LoopVar = 0; LoopVar < (Buffer[1] * Buffer[1]); ++LoopVar) {
        if (LoopVar > 0) {
            Buffer[1] = 42;
// CHECK-MESSAGES:[[@LINE-1]]:13: warning: do not mutate the variable 'Buffer' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-4]]:34: note: variable 'Buffer' used in loop condition here
        }
    }

    #pragma omp parallel for
    for (LoopVar = 0; LoopVar < (Buffer[1] * Buffer[1]); ++LoopVar) {
        if (Buffer[0] > 0) {
            doesMutate(Buffer[1]);
// CHECK-MESSAGES:[[@LINE-1]]:24: warning: do not mutate the variable 'Buffer' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-4]]:34: note: variable 'Buffer' used in loop condition here
            doesNotMutate(Buffer[1]);
        }
    }

    #pragma omp parallel for
    for (int LoopVar = 0; LoopVar < (Buffer[1] * Buffer[1]); ++LoopVar) {
        for (int LoopVar2 = 0; LoopVar2 < (Buffer[1] * Buffer[1]); ++LoopVar2) {
            if (Buffer[0] > 0) {
                doesMutate(Buffer[1]);
// CHECK-MESSAGES:[[@LINE-1]]:28: warning: do not mutate the variable 'Buffer' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-5]]:38: note: variable 'Buffer' used in loop condition here
// CHECK-MESSAGES:[[@LINE-6]]:50: note: variable 'Buffer' used in loop condition here
                doesNotMutate(Buffer[1]);
            }
        }
    }

    #pragma omp parallel for collapse(2)
    for (int LoopVar = 0; LoopVar < (Buffer[1] * Buffer[1]); ++LoopVar) {
        for (int LoopVar2 = 0; LoopVar2 < (Buffer[1] * Buffer[1]); ++LoopVar2) {
            if (Buffer[0] > 0) {
                doesMutate(Buffer[1]);
// CHECK-MESSAGES:[[@LINE-1]]:28: warning: do not mutate the variable 'Buffer' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-5]]:38: note: variable 'Buffer' used in loop condition here
// CHECK-MESSAGES:[[@LINE-6]]:50: note: variable 'Buffer' used in loop condition here
                doesNotMutate(Buffer[1]);
            }
        }
    }

// This is actually fine, but detection of this case is hard.
// Would need to statically prove that the index used for the loop bound is
// never modified by the loop body.
    #pragma omp parallel for collapse(2)
    for (int LoopVar = 0; LoopVar < (Buffer[1] * Buffer[1]); ++LoopVar) {
        for (int LoopVar2 = 0; LoopVar2 < (Buffer[1] * Buffer[1]); ++LoopVar2) {
            if (Buffer[0] > 0) {
                doesMutate(Buffer[2]);
// CHECK-MESSAGES:[[@LINE-1]]:28: warning: do not mutate the variable 'Buffer' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-5]]:38: note: variable 'Buffer' used in loop condition here
// CHECK-MESSAGES:[[@LINE-6]]:50: note: variable 'Buffer' used in loop condition here
                doesNotMutate(Buffer[2]);
            }
        }
    }
}

void testBufferValueAsLoopBound(int ** Buffer, int * BufferSizes, int NumBuffers) {
    int LoopVar = 0;

    for (int BufferId = 0; BufferId < NumBuffers; ++BufferId) {
    #pragma omp parallel for
        for (LoopVar = 0; LoopVar < (BufferSizes[BufferId] * BufferSizes[BufferId]); ++LoopVar) {
            if (LoopVar > 0) {
                BufferSizes[BufferId] = 42;
// CHECK-MESSAGES:[[@LINE-1]]:17: warning: do not mutate the variable 'BufferSizes' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-4]]:38: note: variable 'BufferSizes' used in loop condition here
                ++LoopVar;
// CHECK-MESSAGES:[[@LINE-1]]:19: warning: do not mutate the variable 'LoopVar' used as the for statement counter of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
            }
        }

    #pragma omp parallel for
        for (LoopVar = 0; LoopVar < (BufferSizes[BufferId] * BufferSizes[BufferId]); ++LoopVar) {
            if (Buffer[BufferId][0] > 0) {
                doesMutate(BufferSizes[BufferId]);
// CHECK-MESSAGES:[[@LINE-1]]:28: warning: do not mutate the variable 'BufferSizes' used in the for statement condition of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
// CHECK-MESSAGES:[[@LINE-4]]:38: note: variable 'BufferSizes' used in loop condition here
                doesNotMutate(BufferSizes[BufferId]);
                doesMutate(LoopVar);
// CHECK-MESSAGES:[[@LINE-1]]:28: warning: do not mutate the variable 'LoopVar' used as the for statement counter of the OpenMP work-sharing construct [openmp-do-not-modify-loop-variable]
                doesNotMutate(LoopVar);
            }
        }
    }
}
