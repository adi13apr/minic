// RUN: not %minic --dump-ast %s 2> %t.err | FileCheck %s
// RUN: FileCheck --check-prefix=ERR %s < %t.err

// After an error, the parser skips the broken statement or function and
// keeps going, so later code still ends up in the AST.

fn first() -> int {
  let x: int = 1 +;
  let y: int = 2;
  return y;
}
// ERR: error: expected expression, found ';'
// CHECK:      FunctionDecl first() -> int
// CHECK-NEXT:   Block
// CHECK-NEXT:     Let y: int = 2
// CHECK-NEXT:     Return y

fn broken( -> int { }
// ERR: error: expected identifier, found '->'

fn last() -> int { return 0; }
// CHECK-NEXT: FunctionDecl last() -> int