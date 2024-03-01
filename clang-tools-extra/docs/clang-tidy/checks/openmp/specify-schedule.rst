.. title:: clang-tidy - openmp-specify-schedule

openmp-specify-schedule
=======================

Detects OpenMP ``for`` directives without a ``schedule`` clause, because the
default schedule is implementation-defined. The diagnostic is specifically
worded to suggest to the programmer to select the right kind of schedule for
the loop.

Example
-------

.. code-block:: c++

  int Accumulate(int*Buffer, int n){
    int Sum = 0;
    // The default schedule is implementation-defined
    #pragma omp parallel for default(none) firstprivate(n) shared(Buffer) reduction(+:Sum)
    for (int I = 0; I < n; ++I) {
      Sum += Buffer[I];
    }
    return Sum;
  }

  int ConditionalAccumulate(int*Buffer, bool*Flag, int n){
    int Sum = 0;
    // The default schedule is implementation-defined
    #pragma omp parallel for default(none) firstprivate(n) shared(Buffer) reduction(+:Sum)
    for (int I = 0; I < n; ++I) {
      if (Flag[I])
        Sum += Buffer[I];
    }
    return Sum;
  }


In the given example, the computation in ``Accumulate`` suggests that the
``static`` schedule would be the most performant because each iteration does
the same amount of work. However, in the ``ConditionalAccumulate`` function,
the work is dependent on the value in ``Flag[I]``, which suggests a different
schedule.
