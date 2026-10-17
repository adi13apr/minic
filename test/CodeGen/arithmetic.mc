// RUN: %minic --emit-llvm %s | FileCheck %s

fn arith(a: int, b: int) -> int {
  return a + b * 2 - -a;
}
// CHECK-LABEL: define internal i32 @arith(i32 %a, i32 %b) {
// CHECK-NEXT:  entry:
// CHECK-NEXT:    %a.addr = alloca i32
// CHECK-NEXT:    %b.addr = alloca i32
// CHECK-NEXT:    store i32 %a, ptr %a.addr
// CHECK-NEXT:    store i32 %b, ptr %b.addr
// CHECK-NEXT:    [[A:%.*]] = load i32, ptr %a.addr
// CHECK-NEXT:    [[B:%.*]] = load i32, ptr %b.addr
// CHECK-NEXT:    [[MUL:%.*]] = mul i32 [[B]], 2
// CHECK-NEXT:    [[ADD:%.*]] = add i32 [[A]], [[MUL]]
// CHECK-NEXT:    [[A2:%.*]] = load i32, ptr %a.addr
// CHECK-NEXT:    [[NEG:%.*]] = sub i32 0, [[A2]]
// CHECK-NEXT:    [[SUB:%.*]] = sub i32 [[ADD]], [[NEG]]
// CHECK-NEXT:    ret i32 [[SUB]]

// Division is guarded: dividing by zero or INT_MIN / -1 traps.
fn divide(a: int, b: int) -> int {
  return a / b;
}
// CHECK-LABEL: define internal i32 @divide(
// CHECK:         %div.iszero = icmp eq i32 [[DB:%.*]], 0
// CHECK-NEXT:    %div.ismin = icmp eq i32 [[DA:%.*]], -2147483648
// CHECK-NEXT:    %div.isminusone = icmp eq i32 [[DB]], -1
// CHECK-NEXT:    %div.overflow = and i1 %div.ismin, %div.isminusone
// CHECK-NEXT:    %div.invalid = or i1 %div.iszero, %div.overflow
// CHECK-NEXT:    br i1 %div.invalid, label %trap, label %div.ok
// CHECK:       div.ok:
// CHECK-NEXT:    %div = sdiv i32 [[DA]], [[DB]]
// CHECK-NEXT:    ret i32 %div
// CHECK:       trap:
// CHECK-NEXT:    call void @llvm.trap()
// CHECK-NEXT:    unreachable

// A constant divisor other than 0 or -1 needs no guard.
fn divideByConstant(a: int) -> int {
  return a / 4;
}
// CHECK-LABEL: define internal i32 @divideByConstant(
// CHECK-NOT:     icmp
// CHECK:         %div = sdiv i32 {{%.*}}, 4

fn minInt() -> int {
  return -2147483648;
}
// CHECK-LABEL: define internal i32 @minInt() {
// CHECK-NEXT:  entry:
// CHECK-NEXT:    ret i32 -2147483648

fn compare(a: int, b: int) -> bool {
  return a < b == !(a >= b);
}
// CHECK-LABEL: define internal i1 @compare(
// CHECK:         [[LT:%.*]] = icmp slt i32
// CHECK:         [[GE:%.*]] = icmp sge i32
// CHECK-NEXT:    [[NOT:%.*]] = xor i1 [[GE]], true
// CHECK-NEXT:    [[EQ:%.*]] = icmp eq i1 [[LT]], [[NOT]]
// CHECK-NEXT:    ret i1 [[EQ]]

fn main() -> int { return 0; }
// CHECK-LABEL: define i32 @main() {