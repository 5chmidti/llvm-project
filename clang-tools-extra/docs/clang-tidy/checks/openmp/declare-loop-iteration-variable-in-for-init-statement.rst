.. title:: clang-tidy - openmp-declare-loop-iteration-variable-in-for-init-statement

openmp-declare-loop-iteration-variable-in-for-init-statement
============================================================

Finds OpenMP for loops where the loop iteration variable is not declared
in the for loop initialization statement.

Example
-------

.. code-block:: c++

  void foo1(int A) {
      int Var;
      #pragma omp parallel for firstprivate(A) private(Var)
      for (Var = 0; Var < A; ++Var) {}

      // becomes

      #pragma omp parallel for firstprivate(A)
      for (int Var = 0; Var < A; ++Var) {}

  }
