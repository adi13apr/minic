// RUN: not %minic %s 2>&1 | FileCheck %s

fn twice() -> int { return 1; }
fn twice() -> int { return 2; }
// CHECK: redeclaration.mc:[[@LINE-1]]:4: error: redefinition of function 'twice'
// CHECK: redeclaration.mc:[[@LINE-3]]:4: note: previous definition is here

fn dupParam(a: int, a: bool) -> int { return 0; }
// CHECK: redeclaration.mc:[[@LINE-1]]:21: error: redeclaration of 'a'
// CHECK: redeclaration.mc:[[@LINE-2]]:13: note: previous declaration is here

fn main() -> int {
  let x: int = 1;
  let x: int = 2;
// CHECK: redeclaration.mc:[[@LINE-1]]:7: error: redeclaration of 'x'
// CHECK: redeclaration.mc:[[@LINE-3]]:7: note: previous declaration is here
  {
    // Shadowing in an inner block is allowed.
    let x: bool = true;
  }
  return x;
}
// CHECK-NOT: error