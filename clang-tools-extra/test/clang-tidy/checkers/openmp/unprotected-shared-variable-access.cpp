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

int Global = 0;

struct StaticStorage {
    static int StaticMember;
    static int StaticThreadPrivate;
    #pragma omp threadprivate(StaticThreadPrivate)
    thread_local static int StaticThreadLocal;
};
int StaticStorage::StaticMember = 0;
int StaticStorage::StaticThreadPrivate = 0;
thread_local int StaticStorage::StaticThreadLocal = 0;

void var(int* Buffer, int BufferSize) {
    int Sum = 42;
    int Sum2 = 42;
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

        #pragma omp atomic
        Sum += Buffer[LoopVar];

        #pragma omp critical
        Sum += Buffer[LoopVar];
        #pragma omp critical
        doesMutate(Sum);

    }

    #pragma omp parallel
    #pragma omp for
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        #pragma omp atomic update
            Sum += Buffer[LoopVar];
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

    #pragma omp parallel for firstprivate(BufferSize) reduction(+ : Sum) reduction(+ : Sum2)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
        Sum2 += Buffer[LoopVar];
    }

    #pragma omp parallel for firstprivate(BufferSize) reduction(+ : Sum2) reduction(+ : Sum)
    for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
        Sum += Buffer[LoopVar];
        Sum2 += Buffer[LoopVar];
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

    #pragma omp parallel
    {
        #pragma omp critical
        Sum = 10;

        #pragma omp barrier

        #pragma omp for reduction(+ : Sum)
            for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
                Sum += Buffer[LoopVar];
            }
    }

    #pragma omp parallel
    {
        #pragma omp critical
        Sum = 10;

        #pragma omp for reduction(+ : Sum)
            for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
                Sum += Buffer[LoopVar];
            }
    }

    #pragma omp parallel
    {
        #pragma omp master
        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp for reduction(+ : Sum)
            for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
                Sum += Buffer[LoopVar];
            }
    }

    #pragma omp parallel
    #pragma omp master
    Sum = 10;

    #pragma omp parallel
    {
        #pragma omp master
        Sum = 10;

        #pragma omp master
        Sum = 10;
    }

    #pragma omp parallel
    {
        #pragma omp master
        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp master
        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp master
        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp master
        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp critical
        Sum = 10;
    }

    #pragma omp parallel
    {
        #pragma omp single nowait
        Sum = 0;

        auto Val = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        Global = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Global' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        StaticStorage::StaticMember = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'StaticMember' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        StaticStorage::StaticThreadPrivate = 0;
    }

    #pragma omp parallel
    {
        StaticStorage::StaticThreadLocal = 0;
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

void tasks() {
    int Sum = 0;
    int Sum2 = 0;
    #pragma omp parallel
    {
        #pragma omp task
        Sum = 1;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization; specify synchronization on the task with `depend` [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;
    }

    #pragma omp parallel
    {
        #pragma omp task depend(in: Sum)
        auto Val = Sum;

        #pragma omp task depend(inout: Sum)
        ++Sum;

        #pragma omp task depend(out: Sum)
        Sum = 1;
    }

    #pragma omp parallel
    {
        #pragma omp task depend(inout: Sum)
        ++Sum;

        #pragma omp task
        Sum = 1;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization; specify synchronization on the task with `depend` [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp critical
        Sum = 10;
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;

        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;

        #pragma omp taskwait

        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;

        #pragma omp taskwait depend(in: Sum)

        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;

        #pragma omp taskwait depend(in: Sum2)

        Sum = 10;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;

        #pragma omp taskwait

        auto Val = Sum;
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;

        #pragma omp taskwait depend(in: Sum)

        auto Val = Sum;
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;

        #pragma omp taskwait depend(in: Sum2)

        auto Val = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;
        #pragma omp task depend(out: Sum2)
        Sum2 = 1;

        #pragma omp taskwait depend(in: Sum2)

        auto Val = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
        #pragma omp task
            Sum = 1;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: do not access shared variable 'Sum' of type 'int' without synchronization; specify synchronization on the task with `depend` [openmp-unprotected-shared-variable-access]

    #pragma omp parallel
        #pragma omp single
            #pragma omp task
                Sum = 1;
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: do not access shared variable 'Sum' of type 'int' without synchronization; specify synchronization on the task with `depend` [openmp-unprotected-shared-variable-access]

    #pragma omp parallel sections
    {
        #pragma omp task
            Sum = 1;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: do not access shared variable 'Sum' of type 'int' without synchronization; specify synchronization on the task with `depend` [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
            Sum = 1;

        #pragma omp task depend(in: Sum2)
            auto Val = Sum + Sum2;
// CHECK-MESSAGES: :[[@LINE-1]]:24: warning: do not access shared variable 'Sum' of type 'int' without synchronization; specify synchronization on the task with `depend` [openmp-unprotected-shared-variable-access]

        #pragma omp taskwait

        auto Val2 = Sum;
    }

    #pragma omp parallel
    {
        #pragma omp task depend(out: Sum)
        Sum = 1;
        #pragma omp task depend(out: Sum2)
        Sum2 = 1;

        #pragma omp task depend(in: Sum) if(0)
        ;

        auto Val1 = Sum;
        auto Val2 = Sum2;
// CHECK-MESSAGES: :[[@LINE-1]]:21: warning: do not access shared variable 'Sum2' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        int Local = 10;

        #pragma omp task
        Local = 0;

        auto Val = Local;
    }

    #pragma omp parallel
    {
        int Local = 10;

        #pragma omp task shared(Local)
        Local = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Local' of type 'int' without synchronization; specify synchronization on the task with `depend` [openmp-unprotected-shared-variable-access]

        auto Val = Local;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Local' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp parallel
    {
        int Local = 10;

        #pragma omp task depend(out: Local)
        Local = 0;

        auto Val = Local;
    }

    #pragma omp parallel
    {
        int Local = 10;

        #pragma omp task shared(Local) depend(out: Local)
        Local = 0;

        auto Val = Local;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Local' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }
}

void barrier() {
    int Sum = 0;
    #pragma omp parallel
    {
        #pragma omp critical
        Sum = 1;

        #pragma omp barrier

        auto Val = Sum;
    }

    #pragma omp parallel
    {
        #pragma omp critical
        Sum = 1;

        #pragma omp barrier

        auto Val = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp critical
        Sum = 1;
    }

    #pragma omp parallel
    {
        #pragma omp critical
        Sum = 1;

        auto Val = Sum;
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]

        #pragma omp barrier

        #pragma omp critical
        Sum = 1;
    }
}

void target() {
    int Sum = 0;
    #pragma omp target map(to: Sum)
    {
        Sum = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp target teams map(to: Sum)
    {
        Sum = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not access shared variable 'Sum' of type 'int' without synchronization [openmp-unprotected-shared-variable-access]
    }

    #pragma omp target teams distribute parallel for map(to: Sum) reduction(+: Sum)
    for (int i = 0; i < 10; ++i) {
        Sum += i;
    }
}
