// RUN: %minic --dump-ast %s | FileCheck %s

fn add(a: int, b: int) -> int { return a + b; }
// CHECK:      FunctionDecl add(a: int, b: int) -> int
// CHECK-NEXT:   Block
// CHECK-NEXT:     Return (+ a b)

fn greet() { print(1); return; }
// CHECK-NEXT: FunctionDecl greet() -> void
// CHECK-NEXT:   Block
// CHECK-NEXT:     ExprStmt (call print 1)
// CHECK-NEXT:     Return

fn isPositive(x: int) -> bool { return x > 0; }
// CHECK-NEXT: FunctionDecl isPositive(x: int) -> bool
// CHECK-NEXT:   Block
// CHECK-NEXT:     Return (> x 0)