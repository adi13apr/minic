// RUN: %minic --emit-llvm %s | FileCheck %s

// Every variable gets an alloca in the entry block, even one declared in an
// inner block. The inner x shadows the outer one, so it gets its own slot.
fn main() -> int {
  let x: int = 1;
  x = x + 41;
  {
    let x: bool = true;
  }
  return x;
}
// CHECK-LABEL: define i32 @main() {
// CHECK-NEXT:  entry:
// CHECK-NEXT:    %x = alloca i32
// CHECK-NEXT:    %x1 = alloca i1
// CHECK-NEXT:    store i32 1, ptr %x
// CHECK-NEXT:    [[X:%.*]] = load i32, ptr %x
// CHECK-NEXT:    %add = add i32 [[X]], 41
// CHECK-NEXT:    store i32 %add, ptr %x
// CHECK-NEXT:    store i1 true, ptr %x1
// CHECK-NEXT:    [[RET:%.*]] = load i32, ptr %x
// CHECK-NEXT:    ret i32 [[RET]]