// RUN: %minic %examples/fibonacci.mc -o %t.fib && %t.fib | FileCheck %s --match-full-lines --check-prefix=FIB
// RUN: %minic -O2 %examples/primes.mc -o %t.primes && %t.primes | FileCheck %s --match-full-lines --check-prefix=PRIMES
// RUN: %minic -O2 %examples/collatz.mc -o %t.collatz && %t.collatz | FileCheck %s --match-full-lines --check-prefix=COLLATZ

// Keeps every program in examples/ compiling and producing the right output.

// FIB:      0
// FIB-NEXT: 1
// FIB-NEXT: 1
// FIB-NEXT: 2
// FIB-NEXT: 3
// FIB-NEXT: 5
// FIB-NEXT: 8
// FIB-NEXT: 13
// FIB-NEXT: 21
// FIB-NEXT: 34

// PRIMES:      2
// PRIMES-NEXT: 3
// PRIMES-NEXT: 5
// PRIMES-NEXT: 7
// PRIMES-NEXT: 11
// PRIMES-NEXT: 13
// PRIMES-NEXT: 17
// PRIMES-NEXT: 19
// PRIMES-NEXT: 23
// PRIMES-NEXT: 29

// COLLATZ: 111