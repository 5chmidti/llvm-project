.. title:: clang-tidy - openmp-avoid-nesting-critical-sections

openmp-avoid-nesting-critical-sections
======================================

Warns about nested ``critical`` sections.
When nesting ``critical`` sections, it can become possible to create deadlocks
due to different ordering of which ``critical`` sections are locked first.
The check can inspect the bodies of called functions if they are visible.
This check diagnoses any use of nested ``critical`` sections,
while the ``openmp-critical-section-deadlock`` check detects deadlocks.


.. code-block:: c++

    void foo(bool* flags, int N) {
        int x = 0;
        int y = 0;
        const auto CriticalLambda = [](){
            #pragma omp critical
            int x;
        };
        #pragma omp parallel for
        for (int i = 0; i < N; ++i) {
            if (flags[i])
            #pragma omp critical(X)
                #pragma omp critical(Y) // detected nested critical directives
                x = y;
            else
            #pragma omp critical(Y)
                CriticalLambda(); // detected nested critical directives
        }
    }

