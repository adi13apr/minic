// RUN: %minic --emit-llvm -O2 %s | FileCheck %s

// At -O2 the internal function is inlined into main, the call is folded to
// a constant, and the now-unused function is deleted.
fn add(a: int, b: int) -> int { return a + b; }

fn main() -> int { return add(1, 2); }
// CHECK-NOT:   @add
// CHECK:       define {{.*}}i32 @main()
// CHECK-NEXT:  entry:
// CHECK-NEXT:    ret i32 3