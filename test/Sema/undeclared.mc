// RUN: not %minic %s 2>&1 | FileCheck %s

fn main() -> int {
  let a: int = b;
// CHECK: undeclared.mc:[[@LINE-1]]:16: error: use of undeclared variable 'b'
  c = 1;
// CHECK: undeclared.mc:[[@LINE-1]]:3: error: use of undeclared variable 'c'
  let d: int = f(1);
// CHECK: undeclared.mc:[[@LINE-1]]:16: error: call to undeclared function 'f'
  {
    let inner: int = 1;
  }
  return inner;
// CHECK: undeclared.mc:[[@LINE-1]]:10: error: use of undeclared variable 'inner'
}