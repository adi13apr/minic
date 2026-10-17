// RUN: not %minic %s 2>&1 | FileCheck %s

fn main() -> int {
  let x: int = 1;
  x + 1;
// CHECK: expr-stmt.mc:[[@LINE-1]]:3: error: expression result is unused; only function calls can be used as statements
  print(x);
  return 0;
}
// CHECK-NOT: error