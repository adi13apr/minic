// RUN: not %minic %s 2>&1 | FileCheck %s

fn missingSemi() -> int {
  let x: int = 5
// CHECK: errors.mc:[[@LINE-1]]:17: error: expected ';'
  return x;
}

fn badExpr() -> int {
  return 1 + ;
// CHECK: errors.mc:[[@LINE-1]]:14: error: expected expression, found ';'
}

fn unclosedParen() -> int {
  return (1 + 2;
// CHECK: errors.mc:[[@LINE-1]]:16: error: expected ')', found ';'
}

fn badType(x: float) -> int {
// CHECK: errors.mc:[[@LINE-1]]:15: error: expected type, found identifier 'float'
  return 0;
}

fn badParam(a int) -> int {
// CHECK: errors.mc:[[@LINE-1]]:15: error: expected ':', found 'int'
  return 0;
}

fn missingBrace() -> int {
  if (true) {
    return 1;
  return 0;
}

fn afterMissingBrace() -> int {
// CHECK: errors.mc:[[@LINE-1]]:1: error: expected '}', found 'fn'
  return 0;
}

let global: int = 1;
// CHECK: errors.mc:[[@LINE-1]]:1: error: global variables are not supported; declare variables inside a function

fn ok() -> int { return 0; }

42
// CHECK: errors.mc:[[@LINE-1]]:1: error: expected 'fn' at top level, found integer literal '42'