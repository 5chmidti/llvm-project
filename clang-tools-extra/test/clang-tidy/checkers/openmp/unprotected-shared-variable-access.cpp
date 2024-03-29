// RUN: %check_clang_tidy -check-suffix=,NOTMARKED %s openmp-unprotected-shared-variable-access %t -- --extra-arg=-fopenmp
// RUN: %check_clang_tidy %s openmp-unprotected-shared-variable-access %t -- -config="{CheckOptions: {openmp-unprotected-shared-variable-access.ThreadSafeFunctions: 'threadSafeFunctionByValue;threadSafeFunctionByRef;threadSafeFunctionByConstRef'}}" --extra-arg=-fopenmp

using size_t = unsigned long;

namespace std {
    template <typename T>
    struct atomic {
        atomic(T Val) : Val(Val) {}
        atomic& operator+=(const T&) { return *this; }
        atomic& operator+=(const size_t&) { return *this; }
        T load() const noexcept { return Val; }
        void store(const T& NewVal) noexcept { Val = NewVal; }
        T Val;
    };

    template <typename T>
    struct atomic_ref {
        atomic_ref(T &Val) : Val(Val) {}
        atomic_ref& operator+=(const T&) { return *this; }
        atomic_ref& operator+=(const size_t&) { return *this; }
        T load() const noexcept { return Val; }
        void store(const T& NewVal) noexcept { Val = NewVal; }
        T Val;
    };
} // namespace std

void doesMutate(int&);
void doesMutate(std::atomic<int>&);
void doesMutateC(_Atomic int&);
void doesNotMutate(const int&);
void doesNotMutate(const std::atomic<int>&);
void doesNotMutateC(const _Atomic int&);

void var(int* Buffer, int BufferSize) {
    int Sum = 42;
    #pragma omp parallel for default(none) shared(Buffer, Sum) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutate(Sum);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutate(Sum);
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        const int SavedSum = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma barrier

        #pragma omp atomic
        Sum += Buffer[LoopVar];

        #pragma omp critical
        Sum += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Sum);

// CHECK-MESSAGES: :[[@LINE-20]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-19]]:20: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-9]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-7]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-6]]:20: note: 'Sum' was mutated here
    }

    #pragma omp parallel for
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutate(Sum);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutate(Sum);
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        const int SavedSum = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp atomic
        Sum += Buffer[LoopVar];

        #pragma omp critical
        Sum += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Sum);

// CHECK-MESSAGES: :[[@LINE-18]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-17]]:20: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-9]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-7]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-6]]:20: note: 'Sum' was mutated here
    }

    #pragma omp parallel for
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutate(Sum);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutate(Sum);
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        const int SavedSum = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp atomic
        Sum += Buffer[LoopVar];

        #pragma omp critical
        Sum += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Sum);

// CHECK-MESSAGES: :[[@LINE-18]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-17]]:20: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-9]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-7]]:9: note: 'Sum' was mutated here
// CHECK-MESSAGES: :[[@LINE-6]]:20: note: 'Sum' was mutated here
    }

    #pragma omp parallel
        #pragma omp atomic write
            Sum = 10;

    #pragma omp parallel for default(none) shared(Buffer, Sum) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Sum);
        const int SavedSum = Sum;
    }

    #pragma omp parallel default(none) shared(Buffer) firstprivate(BufferSize) reduction(+ : Sum)
    #pragma omp parallel for reduction(+ : Sum)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer) firstprivate(BufferSize) reduction(+ : Sum)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
    }

    #pragma omp parallel for default(shared) shared(Buffer) firstprivate(BufferSize) reduction(+ : Sum)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
    }

    #pragma omp parallel default(shared) shared(Buffer) firstprivate(BufferSize)
    #pragma omp for reduction(+ : Sum)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
    }

    #pragma omp parallel default(none) shared(Buffer, Sum) firstprivate(BufferSize)
    #pragma omp for reduction(+ : Sum)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
    }

    #pragma omp parallel
    {
        int LocalSum = 0;
        #pragma omp for
        for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
            LocalSum += Buffer[LoopVar];
        }
    }
}

void varAtomic(int* Buffer, int BufferSize) {
    std::atomic<int> Sum = 42;
    #pragma omp parallel for default(none) shared(Buffer, Sum) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
        doesMutate(Sum);
        doesNotMutate(Sum);

        const auto SavedSum = Sum;

        Sum += Buffer[LoopVar];

        #pragma omp critical
        Sum += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Sum);
    }

    #pragma omp parallel for default(none) shared(Buffer, Sum) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Sum);
        const auto SavedSum = Sum;
    }
}

void varAtomicC(int* Buffer, int BufferSize) {
    _Atomic int Sum = 42;
    #pragma omp parallel for default(none) shared(Buffer, Sum) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
        doesMutateC(Sum);
        doesNotMutateC(Sum);

        const auto SavedSum = Sum;

        Sum += Buffer[LoopVar];

        #pragma omp critical
        Sum += Buffer[LoopVar];
        #pragma omp critical
        doesMutateC(Sum);
    }

    #pragma omp parallel for default(none) shared(Buffer, Sum) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutateC(Sum);
        const auto SavedSum = Sum;
    }
}

void bufferAccess(int* Buffer, int* Buffer2, int BufferSize) {
    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2[LoopVar]);
        doesNotMutate(Buffer2[LoopVar]);

        const int SavedVal = Buffer2[LoopVar];

        #pragma omp atomic
        Buffer2[LoopVar] += Buffer[LoopVar];

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]

        const int SavedVal = Buffer2[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp atomic
        Buffer2[LoopVar] += Buffer[LoopVar];

        Buffer2 += 1;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:9: note: 'Buffer2' was mutated here

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[0] += LoopVar;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:9: note: 'Buffer2' was mutated here
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        auto Val = Buffer[0];
    }
}

void bufferAccessAtomic(int* Buffer, std::atomic<int>* Buffer2, int BufferSize) {
    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2[LoopVar]);
        doesNotMutate(Buffer2[LoopVar]);

        const auto SavedVal = Buffer2[LoopVar];

        Buffer2[LoopVar] += Buffer[LoopVar];

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const auto SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2[LoopVar]);
        doesNotMutate(Buffer2[LoopVar]);

        const auto SavedVal = Buffer2[LoopVar];

        Buffer2[LoopVar] += Buffer[LoopVar];

        Buffer2 += 1;

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const auto SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[0] += LoopVar;
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2 += 1;
        Buffer2[0] += LoopVar;
    }
}

void bufferAccessAtomicPtr(int* Buffer, std::atomic<int*> Buffer2, int BufferSize) {
    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2.load()[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2.load()[LoopVar]);
        doesNotMutate(Buffer2.load()[LoopVar]);

        const auto SavedVal = Buffer2.load()[LoopVar];

        Buffer2.load()[LoopVar] += Buffer[LoopVar];

        #pragma omp critical
        Buffer2.load()[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2.load()[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2.load()[LoopVar]);
        const auto SavedVal = Buffer2.load()[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2.load()[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2.load()[LoopVar]);
        doesNotMutate(Buffer2.load()[LoopVar]);

        const auto SavedVal = Buffer2.load()[LoopVar];

        Buffer2.load()[LoopVar] += Buffer[LoopVar];

        Buffer2 += 1;
        #pragma omp critical
        Buffer2.load()[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2.load()[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2.load()[LoopVar]);
        const auto SavedVal = Buffer2.load()[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2.load()[0] += LoopVar;
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2 += 1;
        Buffer2.load()[0] += LoopVar;
    }
}

void bufferAccessAtomicRef(int* Buffer, int* Buffer2, int BufferSize) {
    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2[LoopVar]);
        doesNotMutate(Buffer2[LoopVar]);

        const auto SavedVal = Buffer2[LoopVar];

        Buffer2[LoopVar] += Buffer[LoopVar];

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const auto SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]

        const auto SavedVal = Buffer2[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]

        Buffer2[LoopVar] += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]

        std::atomic_ref<int*>{Buffer2} += 1;
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:31: note: 'Buffer2' was mutated here

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const auto SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[0] += LoopVar;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:9: note: 'Buffer2' was mutated here
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        std::atomic_ref<int*>{Buffer2}.load()[0] += LoopVar;
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:31: note: 'Buffer2' was mutated here
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        std::atomic_ref<int*>{Buffer2} += 1;
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
        // FIXME: false-negative
        Buffer2[0] += LoopVar;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'int *' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-5]]:31: note: 'Buffer2' was mutated here
    }
}

void bufferAccessAtomicC(int* Buffer, _Atomic int* Buffer2, int BufferSize) {
    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
        doesMutateC(Buffer2[LoopVar]);
        doesNotMutateC(Buffer2[LoopVar]);

        const auto SavedVal = Buffer2[LoopVar];

        Buffer2[LoopVar] += Buffer[LoopVar];

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutateC(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutateC(Buffer2[LoopVar]);
        const auto SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type '_Atomic(int) *' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutateC(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:21: warning: do not access shared variable 'Buffer2' of type '_Atomic(int) *' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutateC(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:24: warning: do not access shared variable 'Buffer2' of type '_Atomic(int) *' without synchronization [openmp-unprotected-shared-variable-access]

        const auto SavedVal = Buffer2[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: do not access shared variable 'Buffer2' of type '_Atomic(int) *' without synchronization [openmp-unprotected-shared-variable-access]

        Buffer2[LoopVar] += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type '_Atomic(int) *' without synchronization [openmp-unprotected-shared-variable-access]

        Buffer2 += 1;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type '_Atomic(int) *' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:9: note: 'Buffer2' was mutated here

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutateC(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutateC(Buffer2[LoopVar]);
        const auto SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[0] += LoopVar;
    }
}

namespace std {
template <typename T, size_t N>
struct array{
    const T& operator[](size_t i) const& ;
    T& operator[](size_t i) &;
    T&& operator[](size_t i) &&;

    size_t size() const;

    void modifies();

    T arr[N];
};

template <typename T>
struct vector{
    const T& operator[](size_t i) const& ;
    T& operator[](size_t i) &;
    T&& operator[](size_t i) &&;

    size_t size() const;

    const T* begin() const&;
    T* begin() &;
    T* begin() &&;

    const T* end() const&;
    T* end() &;
    T* end() &&;

    void modifies();
};
} // namespace std

void containerAccess(std::vector<int> Buffer, std::vector<int> Buffer2, size_t BufferSize) {
        #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2[LoopVar]);
        doesNotMutate(Buffer2[LoopVar]);

        const int SavedVal = Buffer2[LoopVar];

        #pragma omp atomic
        Buffer2[LoopVar] += Buffer[LoopVar];

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'std::vector<int>' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Buffer2' of type 'std::vector<int>' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: do not access shared variable 'Buffer2' of type 'std::vector<int>' without synchronization [openmp-unprotected-shared-variable-access]

        const int SavedVal = Buffer2[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: do not access shared variable 'Buffer2' of type 'std::vector<int>' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp atomic
        Buffer2[LoopVar] += Buffer[LoopVar];

        Buffer2.begin();
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'std::vector<int>' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:9: note: 'Buffer2' was mutated here

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[0] += LoopVar;
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        auto Val = Buffer[0];
    }
}

void containerAccessConst(const std::vector<int> Buffer, const std::vector<int> Buffer2, size_t BufferSize) {
        #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);

        const int SavedVal = Buffer2[LoopVar];

        #pragma omp critical
        doesNotMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);

        const int SavedVal = Buffer2[LoopVar];

        Buffer2.begin();

        #pragma omp critical
        doesNotMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        auto Val = Buffer[0];
    }
}

void arrayAccess(std::array<int, 100> Buffer, std::array<int, 100> Buffer2, size_t BufferSize) {
        #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
        doesMutate(Buffer2[LoopVar]);
        doesNotMutate(Buffer2[LoopVar]);

        const int SavedVal = Buffer2[LoopVar];

        #pragma omp atomic
        Buffer2[LoopVar] += Buffer[LoopVar];

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[LoopVar] += Buffer[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'std::array<int, 100>' without synchronization [openmp-unprotected-shared-variable-access]
        doesMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Buffer2' of type 'std::array<int, 100>' without synchronization [openmp-unprotected-shared-variable-access]
        doesNotMutate(Buffer2[LoopVar]);
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: do not access shared variable 'Buffer2' of type 'std::array<int, 100>' without synchronization [openmp-unprotected-shared-variable-access]

        const int SavedVal = Buffer2[LoopVar];
// CHECK-MESSAGES: :[[@LINE-1]]:30: warning: do not access shared variable 'Buffer2' of type 'std::array<int, 100>' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp atomic
        Buffer2[LoopVar] += Buffer[LoopVar];

        Buffer2.modifies();
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Buffer2' of type 'std::array<int, 100>' without synchronization [openmp-unprotected-shared-variable-access]
// CHECK-MESSAGES: :[[@LINE-2]]:9: note: 'Buffer2' was mutated here

        #pragma omp critical
        Buffer2[LoopVar] += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Buffer2[0] += LoopVar;
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        auto Val = Buffer[0];
    }
}

void arrayAccessConst(const std::array<int, 100> Buffer, const std::array<int, 100> Buffer2, size_t BufferSize) {
        #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);

        const int SavedVal = Buffer2[LoopVar];

        #pragma omp critical
        doesNotMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);

        const int SavedVal = Buffer2[LoopVar];

        Buffer2.size();

        #pragma omp critical
        doesNotMutate(Buffer2[LoopVar]);
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        doesNotMutate(Buffer2[LoopVar]);
        const int SavedVal = Buffer2[LoopVar];
    }

    #pragma omp parallel for default(none) shared(Buffer, Buffer2) firstprivate(BufferSize)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        auto Val = Buffer[0];
    }
}

struct Type {};
void threadSafeFunctionByValue(Type);
void threadSafeFunctionByRef(Type&);
void threadSafeFunctionByConstRef(const Type&);

void threadSafeFunctionByValue2(int, Type);
void threadSafeFunctionByRef2(int, Type&);
void threadSafeFunctionByConstRef2(int, const Type&);

void threadSafeFunctions(Type value) {
    #pragma omp parallel
    {
        threadSafeFunctionByValue(value);
// CHECK-MESSAGES-NOTMARKED: :[[@LINE-1]]:35: warning: do not access shared variable 'value' of type 'Type' without synchronization [openmp-unprotected-shared-variable-access]
        threadSafeFunctionByRef(value);
// CHECK-MESSAGES-NOTMARKED: :[[@LINE-1]]:33: warning: do not access shared variable 'value' of type 'Type' without synchronization [openmp-unprotected-shared-variable-access]
        threadSafeFunctionByConstRef(value);
// CHECK-MESSAGES-NOTMARKED: :[[@LINE-1]]:38: warning: do not access shared variable 'value' of type 'Type' without synchronization [openmp-unprotected-shared-variable-access]
        threadSafeFunctionByValue2(0, value);
// CHECK-MESSAGES-NOTMARKED: :[[@LINE-1]]:39: warning: do not access shared variable 'value' of type 'Type' without synchronization [openmp-unprotected-shared-variable-access]
        threadSafeFunctionByRef2(0, value);
// CHECK-MESSAGES-NOTMARKED: :[[@LINE-1]]:37: warning: do not access shared variable 'value' of type 'Type' without synchronization [openmp-unprotected-shared-variable-access]
        threadSafeFunctionByConstRef2(0, value);
// CHECK-MESSAGES-NOTMARKED: :[[@LINE-1]]:42: warning: do not access shared variable 'value' of type 'Type' without synchronization [openmp-unprotected-shared-variable-access]
    }
}
