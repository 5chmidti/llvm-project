// RUN: %check_clang_tidy %s openmp-critical-section-deadlock %t -- --extra-arg=-fopenmp
// RUN: %check_clang_tidy -check-suffixes=,BARRIER %s openmp-critical-section-deadlock %t -- -config="{CheckOptions: {openmp-critical-section-deadlock.IgnoreBarriers: true}}" --extra-arg=-fopenmp

void foo();
void fooWithVisibleCritical() {
    #pragma omp critical
        int x;
}
void fooWithVisibleName1Critical() {
    #pragma omp critical(Name1)
        int x;
}
void fooWithVisibleName2Critical() {
    #pragma omp critical(Name2)
        int x;
}
void fooWithVisibleBarrier(){
  #pragma omp barrier
}

const auto CriticalLambda = [](){
  #pragma omp critical
    int x;
};

const auto CriticalLambdaX = [](){
  #pragma omp critical(X)
    int x;
};

const auto CriticalLambdaWithReturn = [](){
  #pragma omp critical
    int x;
  return 0;
};

void Cycle(bool* flags, int N) {
  int x = 0;
  int y = 0;
  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+5]]:7: warning: deadlock by inconsistent ordering of critical sections 'X' and 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+4]]:7: note: critical section 'X' is entered first in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+4]]:9: note: critical section 'Y' is nested inside 'X' in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: critical section 'Y' is entered first in the second section ordering here
// CHECK-MESSAGES: :[[@LINE+6]]:9: note: critical section 'X' is nested inside 'Y' in the second section ordering here
      #pragma omp critical(X)
        #pragma omp critical(Y)
          x = y;
    else
      #pragma omp critical(Y)
        #pragma omp critical(X)
          x = y;
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+5]]:7: warning: deadlock by inconsistent ordering of critical sections 'X' and 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+4]]:7: note: critical section 'X' is entered first in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+7]]:11: note: critical section 'Y' is nested inside 'X' in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+10]]:7: note: critical section 'Y' is entered first in the second section ordering here
// CHECK-MESSAGES: :[[@LINE+11]]:11: note: critical section 'X' is nested inside 'Y' in the second section ordering here
      #pragma omp critical(X)
        {
          #pragma omp critical
            int a = 0;
          #pragma omp critical(Y)
            x = y;
        }
    else
      #pragma omp critical(Y)
        #pragma omp critical(Name)
          #pragma omp critical(X)
            x = y;
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
      #pragma omp critical(Outer)
        {
// CHECK-MESSAGES: :[[@LINE+5]]:11: warning: deadlock by inconsistent ordering of critical sections 'X' and 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+4]]:11: note: critical section 'X' is entered first in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+4]]:13: note: critical section 'Y' is nested inside 'X' in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+5]]:11: note: critical section 'Y' is entered first in the second section ordering here
// CHECK-MESSAGES: :[[@LINE+5]]:13: note: critical section 'X' is nested inside 'Y' in the second section ordering here
          #pragma omp critical(X)
            #pragma omp critical(Y)
              int a = 0;
          #pragma omp critical(Y)
            #pragma omp critical(X)
              x = y;
        }
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+5]]:7: warning: deadlock by inconsistent ordering of critical sections '' and 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+4]]:7: note: critical section '' is entered first in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+5]]:9: note: critical section 'Y' is nested inside '' in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+8]]:7: note: critical section 'Y' is entered first in the second section ordering here
// CHECK-MESSAGES: :[[@LINE+9]]:9: note: critical section '' is nested inside 'Y' in the second section ordering here
      #pragma omp critical
      {
        #pragma omp critical(Y)
          x = y;
      }
    else
      #pragma omp critical(Y)
      {
        #pragma omp critical
          x = y;
      }
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
      #pragma omp critical(X)
        fooWithVisibleCritical();
    else
      #pragma omp critical(Y)
        fooWithVisibleCritical();
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
      #pragma omp critical(X)
        [](){ CriticalLambda(); };
    else
      #pragma omp critical
        [](){ CriticalLambda(); };
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
      #pragma omp critical(X)
        [Tmp = CriticalLambdaWithReturn()](){ };
    else
// CHECK-MESSAGES: :[[@LINE+3]]:7: warning: deadlock while trying to enter a critical section with the same name '' as the already entered critical section here [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :32:3: note: trying to re-enter the critical section '' here
// CHECK-MESSAGES: :[[@LINE+2]]:16: note: critical section '' is reached by calling 'operator()' here
      #pragma omp critical
        [Tmp = CriticalLambdaWithReturn()](){ };
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+7]]:7: warning: deadlock by inconsistent ordering of critical sections 'X' and '' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: critical section 'X' is entered first in the first section ordering here
// CHECK-MESSAGES: :22:3: note: critical section '' is nested inside 'X' in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+5]]:9: note: critical section '' is reached by calling 'operator()' here
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: critical section '' is entered first in the second section ordering here
// CHECK-MESSAGES: :27:3: note: critical section 'X' is nested inside '' in the second section ordering here
// CHECK-MESSAGES: :[[@LINE+5]]:9: note: critical section 'X' is reached by calling 'operator()' here
      #pragma omp critical(X)
        CriticalLambda();
    else
      #pragma omp critical
        CriticalLambdaX();
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
      #pragma omp critical(X)
        foo();
    else
      #pragma omp critical(Y)
        foo();
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
      #pragma omp critical(Name1)
      {
        #pragma omp critical(Name2)
          x = y;
      }
    else
      #pragma omp critical(Name1)
      {
        #pragma omp critical(Name2)
          x = y;
      }
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+7]]:7: warning: deadlock by inconsistent ordering of critical sections 'Name1' and 'Name2' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: critical section 'Name1' is entered first in the first section ordering here
// CHECK-MESSAGES: :14:5: note: critical section 'Name2' is nested inside 'Name1' in the first section ordering here
// CHECK-MESSAGES: :[[@LINE+5]]:9: note: critical section 'Name2' is reached by calling 'fooWithVisibleName2Critical' here
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: critical section 'Name2' is entered first in the second section ordering here
// CHECK-MESSAGES: :10:5: note: critical section 'Name1' is nested inside 'Name2' in the second section ordering here
// CHECK-MESSAGES: :[[@LINE+5]]:9: note: critical section 'Name1' is reached by calling 'fooWithVisibleName1Critical' here
      #pragma omp critical(Name1)
        fooWithVisibleName2Critical();
    else
      #pragma omp critical(Name2)
        fooWithVisibleName1Critical();
  }

  #pragma omp parallel
  {
// CHECK-MESSAGES-BARRIER: :[[@LINE+5]]:5: warning: deadlock by inconsistent ordering of critical sections 'X' and 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES-BARRIER: :[[@LINE+4]]:5: note: critical section 'X' is entered first in the first section ordering here
// CHECK-MESSAGES-BARRIER: :[[@LINE+4]]:7: note: critical section 'Y' is nested inside 'X' in the first section ordering here
// CHECK-MESSAGES-BARRIER: :[[@LINE+8]]:5: note: critical section 'Y' is entered first in the second section ordering here
// CHECK-MESSAGES-BARRIER: :[[@LINE+8]]:7: note: critical section 'X' is nested inside 'Y' in the second section ordering here
    #pragma omp critical(X)
      #pragma omp critical(Y)
        x = y;

    #pragma omp barrier

    #pragma omp critical(Y)
      #pragma omp critical(X)
        x = y;
  }
}

void SelfCycle() {
  #pragma omp parallel
  {
    #pragma omp critical(Name1)
      fooWithVisibleCritical();

// CHECK-MESSAGES: :[[@LINE+3]]:5: warning: deadlock while trying to enter a critical section with the same name 'Name1' as the already entered critical section here [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :10:5: note: trying to re-enter the critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE+2]]:7: note: critical section 'Name1' is reached by calling 'fooWithVisibleName1Critical' here
    #pragma omp critical(Name1)
      fooWithVisibleName1Critical();

    const auto Lambda1 = [](){ fooWithVisibleName1Critical(); };

    #pragma omp critical(Name1)
      Lambda1;

// CHECK-MESSAGES: :[[@LINE+4]]:5: warning: deadlock while trying to enter a critical section with the same name 'Name1' as the already entered critical section here [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :10:5: note: trying to re-enter the critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE+3]]:7: note: critical section 'Name1' is reached by calling 'operator()' here
// CHECK-MESSAGES: :[[@LINE-8]]:32: note: critical section 'Name1' is reached by calling 'fooWithVisibleName1Critical' here
    #pragma omp critical(Name1)
      Lambda1();
  }
}

void BarrierInsideCriticalViaCallStack() {
  #pragma omp parallel
  {
// CHECK-MESSAGES: :[[@LINE+4]]:7: warning: barrier inside critical section '' results in a deadlock because not all threads can reach the barrier [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+2]]:5: note: barrier encountered while inside critical section '' here
// CHECK-MESSAGES: :[[@LINE+2]]:7: note: barrier is reached by calling 'fooWithVisibleBarrier' here
    #pragma omp critical
      fooWithVisibleBarrier();
  }

  const auto Lambda = [](){
// CHECK-MESSAGES: :[[@LINE+8]]:7: warning: barrier inside critical section '' results in a deadlock because not all threads can reach the barrier [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+6]]:5: note: barrier encountered while inside critical section '' here
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: barrier is reached by calling 'fooWithVisibleBarrier' here
// CHECK-MESSAGES: :[[@LINE+10]]:5: warning: barrier inside critical section '' results in a deadlock because not all threads can reach the barrier [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+3]]:5: note: barrier encountered while inside critical section '' here
// CHECK-MESSAGES: :[[@LINE+8]]:5: note: barrier is reached by calling 'operator()' here
// CHECK-MESSAGES: :[[@LINE+2]]:7: note: barrier is reached by calling 'fooWithVisibleBarrier' here
    #pragma omp critical
      fooWithVisibleBarrier();
  };

  #pragma omp parallel
  {
    Lambda();
  }
}
