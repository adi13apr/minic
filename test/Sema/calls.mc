// RUN: not %minic %s 2>&1 | FileCheck %s

fn print(v: int) {}
// CHECK: calls.mc:[[@LINE-1]]:4: error: 'print' is a builtin function and cannot be redefined

fn add(a: int, b: int) -> int { return a + b; }

fn main() -> int {
  let x: int = add(1);
// CHECK: calls.mc:[[@LINE-1]]:16: error: function 'add' expects 2 arguments, but got 1
  let y: int = add(1, true);
// CHECK: calls.mc:[[@LINE-1]]:23: error: argument 2 of call to 'add' has type 'bool', but parameter 'b' has type 'int'
  print(true);
// CHECK: calls.mc:[[@LINE-1]]:9: error: argument 1 of call to 'print' has type 'bool', but parameter 'value' has type 'int'
  print(1, 2);
// CHECK: calls.mc:[[@LINE-1]]:3: error: function 'print' expects 1 argument, but got 2
  print(x + y);
  return 0;
}