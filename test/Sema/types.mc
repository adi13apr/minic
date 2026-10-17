// RUN: not %minic %s 2>&1 | FileCheck %s

fn nothing() {}

fn main() -> int {
  let a: int = 1 + true;
// CHECK: types.mc:[[@LINE-1]]:18: error: invalid operands to binary '+' ('int' and 'bool')
  let b: bool = 1 && true;
// CHECK: types.mc:[[@LINE-1]]:19: error: invalid operands to binary '&&' ('int' and 'bool')
  let c: bool = 1 == true;
// CHECK: types.mc:[[@LINE-1]]:19: error: invalid operands to binary '==' ('int' and 'bool')
  let d: int = -true;
// CHECK: types.mc:[[@LINE-1]]:16: error: invalid operand to unary '-': expected 'int', found 'bool'
  let e: bool = !1;
// CHECK: types.mc:[[@LINE-1]]:17: error: invalid operand to unary '!': expected 'bool', found 'int'
  let f: int = true;
// CHECK: types.mc:[[@LINE-1]]:16: error: cannot initialize variable 'f' of type 'int' with a value of type 'bool'
  a = 1 < 2;
// CHECK: types.mc:[[@LINE-1]]:9: error: cannot assign a value of type 'bool' to variable 'a' of type 'int'
  if (a) {}
// CHECK: types.mc:[[@LINE-1]]:7: error: condition must be of type 'bool', found 'int'
  while (1 + 1) {}
// CHECK: types.mc:[[@LINE-1]]:12: error: condition must be of type 'bool', found 'int'
  let g: int = nothing();
// CHECK: types.mc:[[@LINE-1]]:16: error: cannot initialize variable 'g' of type 'int' with a value of type 'void'
  return a;
}