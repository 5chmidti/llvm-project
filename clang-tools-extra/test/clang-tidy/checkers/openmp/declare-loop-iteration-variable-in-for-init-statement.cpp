// RUN: %check_clang_tidy %s openmp-declare-loop-iteration-variable-in-for-init-statement %t -- --extra-arg=-fopenmp

void foo1(int A) {
    // CHECK-MESSAGES:[[@LINE+3]]:5: warning: declare the loop iteration variable 'DeclaredBefore' in the for-loop initializer statement [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+1]]:9: note: 'DeclaredBefore' was declared here
    int DeclaredBefore;
    #pragma omp parallel for firstprivate(A) private(DeclaredBefore)
    for (DeclaredBefore = 0; DeclaredBefore < A; ++DeclaredBefore) {}

    #pragma omp parallel for firstprivate(A)
    for (int DeclaredInInit = 0; DeclaredInInit < A; ++DeclaredInInit) {}
}

void foo2(int A) {
    // CHECK-MESSAGES:[[@LINE+3]]:5: warning: declare the loop iteration variable 'DeclaredBefore2' in the for-loop initializer statement [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+1]]:9: note: 'DeclaredBefore2' was declared here
    int DeclaredBefore2;
    #pragma omp parallel for firstprivate(A)
    for (DeclaredBefore2 = 0; DeclaredBefore2 < A; ++DeclaredBefore2) {}
}

void foo3(int A) {
    // CHECK-MESSAGES:[[@LINE+3]]:5: warning: declare the loop iteration variable 'DeclaredBefore3' in the for-loop initializer statement [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+1]]:12: note: 'DeclaredBefore3' was declared here
    int B, DeclaredBefore3, C;
    #pragma omp parallel for firstprivate(A) private(B, DeclaredBefore3, C)
    for (DeclaredBefore3 = 0; DeclaredBefore3 < A; ++DeclaredBefore3) {}
}

void foo4(int A) {
    // CHECK-MESSAGES:[[@LINE+4]]:5: warning: declare the loop iteration variable 'DeclaredBefore4' in the for-loop initializer statement [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+2]]:12: note: 'DeclaredBefore4' was declared here
    // CHECK-MESSAGES:[[@LINE+2]]:58: note: 'DeclaredBefore4' has been declared as 'lastprivate'
    int B, DeclaredBefore4, C;
    #pragma omp parallel for firstprivate(A) lastprivate(DeclaredBefore4, B, C)
    for (DeclaredBefore4 = 0; DeclaredBefore4 < (A + 1); ++DeclaredBefore4) {}
}

void foo5(int A) {
    // CHECK-MESSAGES:[[@LINE+4]]:5: warning: declare the loop iteration variables 'DeclaredBefore5' and 'DeclaredBefore5B' in the for-loop initializer statements [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+2]]:12: note: 'DeclaredBefore5' was declared here
    // CHECK-MESSAGES:[[@LINE+1]]:29: note: 'DeclaredBefore5B' was declared here
    int B, DeclaredBefore5, DeclaredBefore5B, C;
    #pragma omp parallel for private(A, B, DeclaredBefore5, DeclaredBefore5B, C) collapse(2)
    for (DeclaredBefore5 = 0; DeclaredBefore5 < (A + 1); ++DeclaredBefore5) {
        for (DeclaredBefore5B = 0; DeclaredBefore5B < (A + 1); ++DeclaredBefore5B) {
        }
    }
}

void foo6(int A) {
    // CHECK-MESSAGES:[[@LINE+4]]:5: warning: declare the loop iteration variables 'DeclaredBefore6' and 'DeclaredBefore6B' in the for-loop initializer statements [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+2]]:12: note: 'DeclaredBefore6' was declared here
    // CHECK-MESSAGES:[[@LINE+1]]:29: note: 'DeclaredBefore6B' was declared here
    int B, DeclaredBefore6, DeclaredBefore6B, C;
    #pragma omp parallel for private(DeclaredBefore6, DeclaredBefore6B) collapse(2)
    for (DeclaredBefore6 = 0; DeclaredBefore6 < (A + 1); ++DeclaredBefore6) {
        for (DeclaredBefore6B = 0; DeclaredBefore6B < (A + 1); ++DeclaredBefore6B) {
        }
    }
}

void foo7(int A) {
    // CHECK-MESSAGES:[[@LINE+4]]:5: warning: declare the loop iteration variable 'DeclaredBefore7' in the for-loop initializer statement [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+1]]:9: note: 'DeclaredBefore7' was declared here
    int DeclaredBefore7;
    #pragma omp parallel private(DeclaredBefore7)
    #pragma omp for
    for (DeclaredBefore7 = 0; DeclaredBefore7 < A; ++DeclaredBefore7) {}

    int DeclaredBefore7B;
    #pragma omp parallel private(DeclaredBefore7B)
    for (DeclaredBefore7B = 0; DeclaredBefore7B < A; ++DeclaredBefore7B) {}
}

void foo8(int A) {
    // CHECK-MESSAGES:[[@LINE+5]]:9: warning: declare the loop iteration variable 'DeclaredInside8' in the for-loop initializer statement [openmp-declare-loop-iteration-variable-in-for-init-statement]
    // CHECK-MESSAGES:[[@LINE+3]]:13: note: 'DeclaredInside8' was declared here
    #pragma omp parallel
    {
        int DeclaredInside8;
        #pragma omp for
        for (DeclaredInside8 = 0; DeclaredInside8 < A; ++DeclaredInside8) {}
    }

    #pragma omp parallel
    {
        #pragma omp for
        for (int DeclaredInside8B = 0; DeclaredInside8B < A; ++DeclaredInside8B) {}
    }

    #pragma omp parallel
    {
        for (int DeclaredInside8B = 0; DeclaredInside8B < A; ++DeclaredInside8B) {}
    }
}

void foo9() {
    double Buffer[] = {0.0, 3.14, 42.0};
    #pragma omp parallel for
    for (auto Val : Buffer) {}

    #pragma omp parallel
    #pragma omp for
    for (auto Val : Buffer) {}

    #pragma omp parallel
    for (auto Val : Buffer) {}
}


void foo10(int A) {
    #pragma omp parallel for private(A) collapse(2)
    for (int DeclaredBefore10 = 0; DeclaredBefore10 < (A + 1); ++DeclaredBefore10) {
        for (int DeclaredBefore10B = 0; DeclaredBefore10B < (A + 1); ++DeclaredBefore10B) {
            int DeclaredBefore10C;
            for (DeclaredBefore10C = 0; DeclaredBefore10C < (A + 1); ++DeclaredBefore10C) {
            }
        }
    }

    double Buffer[] = {0.0, 3.14, 42.0};
    #pragma omp parallel for private(A) collapse(2)
    for (auto V : Buffer) {
        for (auto V : Buffer) {
            int DeclaredBefore10C;
            for (DeclaredBefore10C = 0; DeclaredBefore10C < (A + 1); ++DeclaredBefore10C) {
                for (auto V : Buffer) {
                }
            }
        }
    }
}
