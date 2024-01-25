.. title:: clang-tidy - openmp-unprotected-shared-variable-access

openmp-unprotected-shared-variable-access
=========================================

Finds unprotected accesses to shared variables.

Example
-------

.. code-block:: c++

    void foo(int* Buffer, int BufferSize) {
        int Sum = 42;
        #pragma omp parallel for shared(Sum) firstprivate(BufferSize)
        for (int LoopVar = 0; LoopVar < BufferSize; ++LoopVar) {
            Sum += Buffer[LoopVar]; // unprotected

            #pragma omp atomic
            Sum += Buffer[LoopVar]; // protected
        }
    }
