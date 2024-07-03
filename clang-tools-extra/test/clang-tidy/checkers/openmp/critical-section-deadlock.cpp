// RUN: %check_clang_tidy %s openmp-critical-section-deadlock %t -- --extra-arg=-fopenmp

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

void BarrierInsideCriticalViaCallStack() {
  #pragma omp parallel
  {
// CHECK-MESSAGES: :[[@LINE+3]]:5: warning: barrier inside critical section '' results in a deadlock because not all threads can reach the barrier [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+3]]:7: note: barrier is reached by calling 'fooWithVisibleBarrier' here
// CHECK-MESSAGES: :17:3: note: barrier encountered here
    #pragma omp critical
      fooWithVisibleBarrier();
  }

  const auto Lambda = [](){
// CHECK-MESSAGES: :[[@LINE+3]]:5: warning: barrier inside critical section '' results in a deadlock because not all threads can reach the barrier [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+3]]:7: note: barrier is reached by calling 'fooWithVisibleBarrier' here
// CHECK-MESSAGES: :17:3: note: barrier encountered here
    #pragma omp critical
      fooWithVisibleBarrier();
  };

  #pragma omp parallel
  {
    Lambda();
  }
}

void Cycle(bool* flags, int N) {
  int x = 0;
  int y = 0;
  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+5]]:7: warning: inconsistent dependency ordering for critical section 'X'; cycle involving 2 critical section orderings detected: 'X' -> 'Y' -> 'X' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+4]]:7: note: Ordering 1 of 2: 'X' -> 'Y'; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE+4]]:9: note: then enters critical section 'Y' here
// CHECK-MESSAGES: :[[@LINE+11]]:7: note: Ordering 2 of 2: 'Y' -> 'X'; starts here by entering 'Y' first
// CHECK-MESSAGES: :[[@LINE+11]]:9: note: then enters critical section 'X' here
      #pragma omp critical(X)
        #pragma omp critical(Y)
          x = y;
    else
// CHECK-MESSAGES: :[[@LINE+5]]:7: warning: inconsistent dependency ordering for critical section 'Y'; cycle involving 2 critical section orderings detected: 'Y' -> 'X' -> 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+4]]:7: note: Ordering 1 of 2: 'Y' -> 'X'; starts here by entering 'Y' first
// CHECK-MESSAGES: :[[@LINE+4]]:9: note: then enters critical section 'X' here
// CHECK-MESSAGES: :[[@LINE-7]]:7: note: Ordering 2 of 2: 'X' -> 'Y'; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE-7]]:9: note: then enters critical section 'Y' here
      #pragma omp critical(Y)
        #pragma omp critical(X)
          x = y;
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])
// CHECK-MESSAGES: :[[@LINE+6]]:7: warning: inconsistent dependency ordering for critical section 'X'; cycle involving 2 critical section orderings detected: 'X' -> 'Y' -> 'Name' -> 'X' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+5]]:7: note: Ordering 1 of 2: 'X' -> 'Y'; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE+8]]:11: note: then enters critical section 'Y' here
// CHECK-MESSAGES: :[[@LINE+24]]:7: note: Ordering 2 of 2: 'Y' -> 'Name' -> 'X'; starts here by entering 'Y' first
// CHECK-MESSAGES: :[[@LINE+24]]:9: note: then enters critical section 'Name' here
// CHECK-MESSAGES: :[[@LINE+24]]:11: note: then enters critical section 'X' here
      #pragma omp critical(X)
        {
          #pragma omp critical
            int a = 0;
          #pragma omp critical(Y)
            x = y;
        }
    else
// CHECK-MESSAGES: :[[@LINE+13]]:7: warning: inconsistent dependency ordering for critical section 'Y'; cycle involving 2 critical section orderings detected: 'Y' -> 'Name' -> 'X' -> 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+12]]:7: note: Ordering 1 of 2: 'Y' -> 'Name' -> 'X'; starts here by entering 'Y' first
// CHECK-MESSAGES: :[[@LINE+12]]:9: note: then enters critical section 'Name' here
// CHECK-MESSAGES: :[[@LINE+12]]:11: note: then enters critical section 'X' here
// CHECK-MESSAGES: :[[@LINE-12]]:7: note: Ordering 2 of 2: 'X' -> 'Y'; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE-9]]:11: note: then enters critical section 'Y' here
// CHECK-MESSAGES: :[[@LINE+8]]:9: warning: inconsistent dependency ordering for critical section 'Name'; cycle involving 3 critical section orderings detected: 'Name' -> 'X' -> 'Y' -> 'Name' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+7]]:9: note: Ordering 1 of 3: 'Name' -> 'X'; starts here by entering 'Name' first
// CHECK-MESSAGES: :[[@LINE+7]]:11: note: then enters critical section 'X' here
// CHECK-MESSAGES: :[[@LINE-17]]:7: note: Ordering 2 of 3: 'X' -> 'Y'; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE-14]]:11: note: then enters critical section 'Y' here
// CHECK-MESSAGES: :[[@LINE+2]]:7: note: Ordering 3 of 3: 'Y' -> 'Name'; starts here by entering 'Y' first
// CHECK-MESSAGES: :[[@LINE+2]]:9: note: then enters critical section 'Name' here
      #pragma omp critical(Y)
        #pragma omp critical(Name)
          #pragma omp critical(X)
            x = y;
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
      #pragma omp critical(Outer)
        {
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
// CHECK-MESSAGES: :[[@LINE+17]]:7: warning: deadlock while trying to enter a critical section with the same name '' as the already entered critical section here [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+17]]:16: note: then calls 'operator()' here
// CHECK-MESSAGES: :31:3: note: then tries to re-enter the critical section '' here
// CHECK-MESSAGES: :[[@LINE+14]]:7: warning: inconsistent dependency ordering for critical section ''; cycle involving 2 critical section orderings detected: '' -> '' -> '' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+13]]:7: note: Ordering 1 of 2: '' -> ''; starts here by entering '' first
// CHECK-MESSAGES: :[[@LINE+13]]:16: note: then calls 'operator()' here
// CHECK-MESSAGES: :31:3: note: then enters critical section '' here
// CHECK-MESSAGES: :[[@LINE+10]]:7: note: Ordering 2 of 2: '' -> ''; starts here by entering '' first
// CHECK-MESSAGES: :[[@LINE+10]]:16: note: then calls 'operator()' here
// CHECK-MESSAGES: :31:3: note: then enters critical section '' here
// CHECK-MESSAGES: :[[@LINE+7]]:7: warning: inconsistent dependency ordering for critical section ''; cycle involving 2 critical section orderings detected: 'X' -> '' -> '' -> '' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: Ordering 1 of 2: '' -> ''; starts here by entering '' first
// CHECK-MESSAGES: :[[@LINE+6]]:16: note: then calls 'operator()' here
// CHECK-MESSAGES: :31:3: note: then enters critical section '' here
// CHECK-MESSAGES: :[[@LINE+3]]:7: note: Ordering 2 of 2: '' -> ''; starts here by entering '' first
// CHECK-MESSAGES: :[[@LINE+3]]:16: note: then calls 'operator()' here
// CHECK-MESSAGES: :31:3: note: then enters critical section '' here
      #pragma omp critical
        [Tmp = CriticalLambdaWithReturn()](){ };
  }

  #pragma omp parallel for
  for (int i = 0; i < N; ++i) {
    if (flags[i])

// CHECK-MESSAGES: :[[@LINE+14]]:7: warning: inconsistent dependency ordering for critical section 'X'; cycle involving 2 critical section orderings detected: 'X' -> '' -> 'X' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+13]]:7: note: Ordering 1 of 2: 'X' -> ''; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE+13]]:9: note: then calls 'operator()' here
// CHECK-MESSAGES: :21:3: note: then enters critical section '' here
// CHECK-MESSAGES: :[[@LINE+13]]:7: note: Ordering 2 of 2: '' -> 'X'; starts here by entering '' first
// CHECK-MESSAGES: :[[@LINE+13]]:9: note: then calls 'operator()' here
// CHECK-MESSAGES: :26:3: note: then enters critical section 'X' here
// CHECK-MESSAGES: :[[@LINE+10]]:7: warning: inconsistent dependency ordering for critical section ''; cycle involving 2 critical section orderings detected: '' -> 'X' -> '' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+9]]:7: note: Ordering 1 of 2: '' -> 'X'; starts here by entering '' first
// CHECK-MESSAGES: :[[@LINE+9]]:9: note: then calls 'operator()' here
// CHECK-MESSAGES: :26:3: note: then enters critical section 'X' here
// CHECK-MESSAGES: :[[@LINE+3]]:7: note: Ordering 2 of 2: 'X' -> ''; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE+3]]:9: note: then calls 'operator()' here
// CHECK-MESSAGES: :21:3: note: then enters critical section '' here
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
// CHECK-MESSAGES: :[[@LINE+14]]:7: warning: inconsistent dependency ordering for critical section 'Name1'; cycle involving 2 critical section orderings detected: 'Name1' -> 'Name2' -> 'Name1' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+13]]:7: note: Ordering 1 of 2: 'Name1' -> 'Name2'; starts here by entering 'Name1' first
// CHECK-MESSAGES: :[[@LINE+13]]:9: note: then calls 'fooWithVisibleName2Critical' here
// CHECK-MESSAGES: :13:5: note: then enters critical section 'Name2' here
// CHECK-MESSAGES: :[[@LINE+13]]:7: note: Ordering 2 of 2: 'Name2' -> 'Name1'; starts here by entering 'Name2' first
// CHECK-MESSAGES: :[[@LINE+13]]:9: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then enters critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE+10]]:7: warning: inconsistent dependency ordering for critical section 'Name2'; cycle involving 2 critical section orderings detected: 'Name2' -> 'Name1' -> 'Name2' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+9]]:7: note: Ordering 1 of 2: 'Name2' -> 'Name1'; starts here by entering 'Name2' first
// CHECK-MESSAGES: :[[@LINE+9]]:9: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then enters critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE+3]]:7: note: Ordering 2 of 2: 'Name1' -> 'Name2'; starts here by entering 'Name1' first
// CHECK-MESSAGES: :[[@LINE+3]]:9: note: then calls 'fooWithVisibleName2Critical' here
// CHECK-MESSAGES: :13:5: note: then enters critical section 'Name2' here
      #pragma omp critical(Name1)
        fooWithVisibleName2Critical();
    else
      #pragma omp critical(Name2)
        fooWithVisibleName1Critical();
  }

  #pragma omp parallel
  {
// CHECK-MESSAGES: :[[@LINE+10]]:5: warning: inconsistent dependency ordering for critical section 'X'; cycle involving 2 critical section orderings detected: 'X' -> 'Y' -> 'X' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+9]]:5: note: Ordering 1 of 2: 'X' -> 'Y'; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE+9]]:7: note: then enters critical section 'Y' here
// CHECK-MESSAGES: :[[@LINE+13]]:5: note: Ordering 2 of 2: 'Y' -> 'X'; starts here by entering 'Y' first
// CHECK-MESSAGES: :[[@LINE+13]]:7: note: then enters critical section 'X' here
// CHECK-MESSAGES: :[[@LINE+11]]:5: warning: inconsistent dependency ordering for critical section 'Y'; cycle involving 2 critical section orderings detected: 'Y' -> 'X' -> 'Y' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+10]]:5: note: Ordering 1 of 2: 'Y' -> 'X'; starts here by entering 'Y' first
// CHECK-MESSAGES: :[[@LINE+10]]:7: note: then enters critical section 'X' here
// CHECK-MESSAGES: :[[@LINE+2]]:5: note: Ordering 2 of 2: 'X' -> 'Y'; starts here by entering 'X' first
// CHECK-MESSAGES: :[[@LINE+2]]:7: note: then enters critical section 'Y' here
    #pragma omp critical(X)
      #pragma omp critical(Y)
        x = y;

    #pragma omp barrier

    #pragma omp critical(Y)
      #pragma omp critical(X)
        x = y;
  }

  #pragma omp parallel
  {
// CHECK-MESSAGES: :[[@LINE+21]]:5: warning: inconsistent dependency ordering for critical section 'A'; cycle involving 3 critical section orderings detected: 'A' -> 'B' -> 'C' -> 'A' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+20]]:5: note: Ordering 1 of 3: 'A' -> 'B'; starts here by entering 'A' first
// CHECK-MESSAGES: :[[@LINE+20]]:7: note: then enters critical section 'B' here
// CHECK-MESSAGES: :[[@LINE+22]]:5: note: Ordering 2 of 3: 'B' -> 'C'; starts here by entering 'B' first
// CHECK-MESSAGES: :[[@LINE+22]]:7: note: then enters critical section 'C' here
// CHECK-MESSAGES: :[[@LINE+24]]:5: note: Ordering 3 of 3: 'C' -> 'A'; starts here by entering 'C' first
// CHECK-MESSAGES: :[[@LINE+24]]:7: note: then enters critical section 'A' here
// CHECK-MESSAGES: :[[@LINE+18]]:5: warning: inconsistent dependency ordering for critical section 'B'; cycle involving 3 critical section orderings detected: 'B' -> 'C' -> 'A' -> 'B' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+17]]:5: note: Ordering 1 of 3: 'B' -> 'C'; starts here by entering 'B' first
// CHECK-MESSAGES: :[[@LINE+17]]:7: note: then enters critical section 'C' here
// CHECK-MESSAGES: :[[@LINE+19]]:5: note: Ordering 2 of 3: 'C' -> 'A'; starts here by entering 'C' first
// CHECK-MESSAGES: :[[@LINE+19]]:7: note: then enters critical section 'A' here
// CHECK-MESSAGES: :[[@LINE+9]]:5: note: Ordering 3 of 3: 'A' -> 'B'; starts here by entering 'A' first
// CHECK-MESSAGES: :[[@LINE+9]]:7: note: then enters critical section 'B' here
// CHECK-MESSAGES: :[[@LINE+15]]:5: warning: inconsistent dependency ordering for critical section 'C'; cycle involving 3 critical section orderings detected: 'C' -> 'A' -> 'B' -> 'C' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+14]]:5: note: Ordering 1 of 3: 'C' -> 'A'; starts here by entering 'C' first
// CHECK-MESSAGES: :[[@LINE+14]]:7: note: then enters critical section 'A' here
// CHECK-MESSAGES: :[[@LINE+4]]:5: note: Ordering 2 of 3: 'A' -> 'B'; starts here by entering 'A' first
// CHECK-MESSAGES: :[[@LINE+4]]:7: note: then enters critical section 'B' here
// CHECK-MESSAGES: :[[@LINE+6]]:5: note: Ordering 3 of 3: 'B' -> 'C'; starts here by entering 'B' first
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: then enters critical section 'C' here
    #pragma omp critical(A)
      #pragma omp critical(B)
        x = 0;

    #pragma omp critical(B)
      #pragma omp critical(C)
        x = 1;

    #pragma omp critical(C)
      #pragma omp critical(A)
        x  = 2;
  }
}

void SelfCycle() {
  #pragma omp parallel
  {
    #pragma omp critical(Name1)
      fooWithVisibleCritical();

// CHECK-MESSAGES: :[[@LINE+10]]:5: warning: deadlock while trying to enter a critical section with the same name 'Name1' as the already entered critical section here [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+10]]:7: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then tries to re-enter the critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE+7]]:5: warning: inconsistent dependency ordering for critical section 'Name1'; cycle involving 2 critical section orderings detected: 'Name1' -> 'Name1' -> 'Name1' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+6]]:5: note: Ordering 1 of 2: 'Name1' -> 'Name1'; starts here by entering 'Name1' first
// CHECK-MESSAGES: :[[@LINE+6]]:7: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then enters critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE+3]]:5: note: Ordering 2 of 2: 'Name1' -> 'Name1'; starts here by entering 'Name1' first
// CHECK-MESSAGES: :[[@LINE+3]]:7: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then enters critical section 'Name1' here
    #pragma omp critical(Name1)
      fooWithVisibleName1Critical();

    const auto Lambda1 = [](){ fooWithVisibleName1Critical(); };

    #pragma omp critical(Name1)
      Lambda1;

// CHECK-MESSAGES: :[[@LINE+12]]:5: warning: deadlock while trying to enter a critical section with the same name 'Name1' as the already entered critical section here [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+12]]:7: note: then calls 'operator()' here
// CHECK-MESSAGES: :[[@LINE-7]]:32: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then tries to re-enter the critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE+8]]:5: warning: inconsistent dependency ordering for critical section 'Name1'; cycle involving 2 critical section orderings detected: 'Name1' -> 'Name1' -> 'Name1' [openmp-critical-section-deadlock]
// CHECK-MESSAGES: :[[@LINE+7]]:5: note: Ordering 1 of 2: 'Name1' -> 'Name1'; starts here by entering 'Name1' first
// CHECK-MESSAGES: :[[@LINE+7]]:7: note: then calls 'operator()' here
// CHECK-MESSAGES: :[[@LINE-12]]:32: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then enters critical section 'Name1' here
// CHECK-MESSAGES: :[[@LINE-17]]:5: note: Ordering 2 of 2: 'Name1' -> 'Name1'; starts here by entering 'Name1' first
// CHECK-MESSAGES: :[[@LINE-17]]:7: note: then calls 'fooWithVisibleName1Critical' here
// CHECK-MESSAGES: :9:5: note: then enters critical section 'Name1' here
    #pragma omp critical(Name1)
      Lambda1();
  }
}
