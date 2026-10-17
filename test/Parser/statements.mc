// RUN: %minic --dump-ast %s | FileCheck %s

fn main() -> int {
  let x: int = 5;
  x = x + 1;
  {
    let y: bool = true;
  }
  if (x < 10) {
    x = 10;
  } else if (x == 10) {
    x = 0;
  } else {
    print(x);
  }
  while (x > 0) {
    x = x - 1;
  }
  return x;
}
// CHECK:      FunctionDecl main() -> int
// CHECK-NEXT:   Block
// CHECK-NEXT:     Let x: int = 5
// CHECK-NEXT:     Assign x = (+ x 1)
// CHECK-NEXT:     Block
// CHECK-NEXT:       Let y: bool = true
// CHECK-NEXT:     If (< x 10)
// CHECK-NEXT:       Block
// CHECK-NEXT:         Assign x = 10
// CHECK-NEXT:       Else
// CHECK-NEXT:         If (== x 10)
// CHECK-NEXT:           Block
// CHECK-NEXT:             Assign x = 0
// CHECK-NEXT:           Else
// CHECK-NEXT:             Block
// CHECK-NEXT:               ExprStmt (call print x)
// CHECK-NEXT:     While (> x 0)
// CHECK-NEXT:       Block
// CHECK-NEXT:         Assign x = (- x 1)
// CHECK-NEXT:     Return x