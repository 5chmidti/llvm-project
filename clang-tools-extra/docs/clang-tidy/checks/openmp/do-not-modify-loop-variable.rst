.. title:: clang-tidy - openmp-do-not-modify-loop-variable

openmp-do-not-modify-loop-variable
==================================

Finds openmp loop directives where the loop counter or condition is modified
in the body.

Example
-------

.. code-block:: c++

    void foo(int *Buffer, int BufferSize) {
        #pragma omp parallel for firstprivate(BufferSize)
        for (int LoopVar = 0; LoopVar < (BufferSize * BufferSize); ++LoopVar) {
            if (Buffer[LoopVar] > 0) {
                ++LoopVar; // modified loop variable
            }
        }
    }
