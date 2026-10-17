// RUN: %minic --emit-llvm %s | FileCheck %s

fn add(a: int, b: int) -> int { return a + b; }
// CHECK-LABEL: define internal i32 @add(i32 %a, i32 %b) {

fn show(x: int) { print(x); }
// CHECK-LABEL: define internal void @show(i32 %x) {
// CHECK:         call void @minic_print(i32 {{%.*}})
// CHECK-NEXT:    ret void

fn main() -> int {
  show(add(1, 2));
  return 0;
}
// CHECK-LABEL: define i32 @main() {
// CHECK:         %call = call i32 @add(i32 1, i32 2)
// CHECK-NEXT:    call void @show(i32 %call)
// CHECK-NEXT:    ret i32 0

// CHECK: declare void @minic_print(i32)