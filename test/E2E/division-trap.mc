// RUN: %minic %s -o %t
// RUN: not --crash %t | FileCheck %s --match-full-lines
// RUN: %minic -O2 %s -o %t.O2
// RUN: not --crash %t.O2 | FileCheck %s --match-full-lines

fn divide(a: int, b: int) -> int { return a / b; }

// Division by zero traps. Output printed before the trap is not lost, and
// nothing after it runs.
fn main() -> int {
  print(divide(7, 2));
  print(divide(7, 0));
  print(99);
  return 0;
}
// CHECK:     3
// CHECK-NOT: 99