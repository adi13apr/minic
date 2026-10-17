// RUN: %minic %s -o %t && %t | FileCheck %s --match-full-lines
// RUN: %minic -O2 %s -o %t.O2 && %t.O2 | FileCheck %s --match-full-lines

// Integer arithmetic wraps around (two's complement) at every -O level.
fn main() -> int {
  let max: int = 2147483647;
  print(max + 1);
  let min: int = -2147483648;
  print(-min);
  print(min - 1);
  return 0;
}
// CHECK:      -2147483648
// CHECK-NEXT: -2147483648
// CHECK-NEXT: 2147483647