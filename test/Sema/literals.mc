// RUN: not %minic %s 2>&1 | FileCheck %s

fn main() -> int {
  let max: int = 2147483647;
  let min: int = -2147483648;
// CHECK-NOT: error
  let tooBig: int = 2147483648;
// CHECK: literals.mc:[[@LINE-1]]:21: error: integer literal '2147483648' is too large for type 'int'
  let tooSmall: int = -2147483649;
// CHECK: literals.mc:[[@LINE-1]]:24: error: integer literal '2147483649' is too large for type 'int'
  let huge: int = 99999999999999999999999;
// CHECK: literals.mc:[[@LINE-1]]:19: error: integer literal '99999999999999999999999' is too large for type 'int'
  return 0;
}
// CHECK-NOT: error