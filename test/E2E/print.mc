// RUN: %minic %s -o %t && %t | FileCheck %s --match-full-lines

fn main() -> int {
  print(42);
  print(-7);
  print(1 + 2 * 3);
  return 0;
}
// CHECK:      42
// CHECK-NEXT: -7
// CHECK-NEXT: 7