// RUN: %minic --emit-llvm %s | FileCheck %s

fn pick(c: bool) -> int {
  let r: int = 0;
  if (c) {
    r = 1;
  } else {
    r = 2;
  }
  return r;
}
// CHECK-LABEL: define internal i32 @pick(i1 %c) {
// CHECK:         br i1 {{%.*}}, label %if.then, label %if.else
// CHECK:       if.then:
// CHECK-NEXT:    store i32 1, ptr %r
// CHECK-NEXT:    br label %if.end
// CHECK:       if.else:
// CHECK-NEXT:    store i32 2, ptr %r
// CHECK-NEXT:    br label %if.end
// CHECK:       if.end:
// CHECK-NEXT:    [[R:%.*]] = load i32, ptr %r
// CHECK-NEXT:    ret i32 [[R]]

// Every branch returns, so no if.end block is created at all.
fn sign(x: int) -> int {
  if (x < 0) {
    return -1;
  } else if (x == 0) {
    return 0;
  } else {
    return 1;
  }
}
// CHECK-LABEL: define internal i32 @sign(
// CHECK:         ret i32 -1
// CHECK:         ret i32 0
// CHECK:         ret i32 1
// CHECK-NOT:     if.end
// CHECK-NOT:     unreachable

fn countdown(n: int) -> int {
  let i: int = n;
  while (i > 0) {
    i = i - 1;
  }
  return i;
}
// CHECK-LABEL: define internal i32 @countdown(
// CHECK:         br label %while.cond
// CHECK:       while.cond:
// CHECK:         [[CMP:%.*]] = icmp sgt i32 {{%.*}}, 0
// CHECK-NEXT:    br i1 [[CMP]], label %while.body, label %while.end
// CHECK:       while.body:
// CHECK:         br label %while.cond
// CHECK:       while.end:
// CHECK:         ret i32

// Code after a return is unreachable and is not emitted.
fn early() -> int {
  return 1;
  print(2);
}
// CHECK-LABEL: define internal i32 @early() {
// CHECK-NEXT:  entry:
// CHECK-NEXT:    ret i32 1
// CHECK-NEXT:  }

fn main() -> int { return 0; }
// CHECK-NOT: minic_print