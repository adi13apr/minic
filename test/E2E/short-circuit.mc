// RUN: %minic %s -o %t && %t | FileCheck %s --match-full-lines
// RUN: %minic -O2 %s -o %t.O2 && %t.O2 | FileCheck %s --match-full-lines

// noisy() prints 1 every time it runs, so the output shows exactly when the
// right operand of && and || is evaluated.
fn noisy(value: bool) -> bool {
  print(1);
  return value;
}

fn main() -> int {
  if (false && noisy(true)) {
    print(100);
  }
  if (true || noisy(true)) {
    print(200);
  }
  if (true && noisy(false)) {
    print(300);
  }
  return 0;
}
// CHECK:      200
// CHECK-NEXT: 1
// CHECK-NOT:  {{.+}}