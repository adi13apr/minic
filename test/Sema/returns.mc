// RUN: not %minic %s 2>&1 | FileCheck %s

fn voidWithValue() {
  return 1;
// CHECK: returns.mc:[[@LINE-1]]:10: error: void function 'voidWithValue' cannot return a value
}

fn missingValue() -> int {
  return;
// CHECK: returns.mc:[[@LINE-1]]:3: error: non-void function 'missingValue' must return a value
}

fn wrongType() -> bool {
  return 1;
// CHECK: returns.mc:[[@LINE-1]]:10: error: cannot return a value of type 'int' from function 'wrongType' with return type 'bool'
}

fn ifWithoutElse(x: int) -> int {
// CHECK: returns.mc:[[@LINE-1]]:4: error: non-void function 'ifWithoutElse' does not return a value on every path
  if (x > 0) {
    return 1;
  }
}

// Every branch returns, so this function is fine.
fn allPathsReturn(x: int) -> int {
  if (x > 0) {
    return 1;
  } else if (x < 0) {
    return -1;
  } else {
    return 0;
  }
}

fn main() -> int { return 0; }
// CHECK-NOT: error