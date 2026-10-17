// RUN: %minic %s -o %t && %t | FileCheck %s --match-full-lines
// RUN: %minic -O2 %s -o %t.O2 && %t.O2 | FileCheck %s --match-full-lines

fn factorial(n: int) -> int {
  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

fn fib(n: int) -> int {
  if (n < 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

fn main() -> int {
  print(factorial(10));
  print(fib(20));
  return 0;
}
// CHECK:      3628800
// CHECK-NEXT: 6765