.. title:: clang-tidy - openmp-missing-for-in-parallel-directive-before-for-loop

openmp-missing-for-in-parallel-directive-before-for-loop
========================================================

Finds OpenMP parallel directives where the next statement is a for loop,
and the ``parallel`` directive is not a ``parallel for`` directive.
This case is most likely an error and the directive should have been
``parallel for``. When the code was intended as written, a compound
statement should be used.

Example
-------

.. code-block:: c++

    void foo(int A) {
        // parallel directive over a for statement
        // -> forgot to add `for` after parallel
        // or add compound statement to signal
        // intended behavior
        #pragma omp parallel firstprivate(A)
        for (int I = 0; I < A; ++I) {}
    }
