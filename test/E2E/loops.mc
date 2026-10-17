// RUN: %minic %s -o %t && %t | FileCheck %s --match-full-lines
// RUN: %minic -O2 %s -o %t.O2 && %t.O2 | FileCheck %s --match-full-lines

fn gcd(a: int, b: int) -> int {
  while (b != 0) {
    let t: int = b;
    b = a - a / b * b;
    a = t;
  }
  return a;
}

fn main() -> int {
  print(gcd(48, 18));
  let sum: int = 0;
  let i: int = 1;
  while (i <= 100) {
    sum = sum + i;
    i = i + 1;
  }
  print(sum);
  return 0;
}
// CHECK:      6
// CHECK-NEXT: 5050