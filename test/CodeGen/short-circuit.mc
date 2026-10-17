// RUN: %minic --emit-llvm %s | FileCheck %s

fn both(a: bool, b: bool) -> bool { return a && b; }
// CHECK-LABEL: define internal i1 @both(
// CHECK:         br i1 {{%.*}}, label %and.rhs, label %and.end
// CHECK:       and.rhs:
// CHECK:         br label %and.end
// CHECK:       and.end:
// CHECK-NEXT:    [[AND:%.*]] = phi i1 [ false, %entry ], [ {{%.*}}, %and.rhs ]
// CHECK-NEXT:    ret i1 [[AND]]

fn either(a: bool, b: bool) -> bool { return a || b; }
// CHECK-LABEL: define internal i1 @either(
// CHECK:         br i1 {{%.*}}, label %or.end, label %or.rhs
// CHECK:       or.end:
// CHECK-NEXT:    phi i1 [ true, %entry ], [ {{%.*}}, %or.rhs ]

// The phi must name the block where the nested right operand ended (or.end),
// not the block where it started (and.rhs).
fn nested(a: bool, b: bool, c: bool) -> bool { return a && (b || c); }
// CHECK-LABEL: define internal i1 @nested(
// CHECK:       and.end:
// CHECK-NEXT:    phi i1 [ false, %entry ], [ {{%.*}}, %or.end ]

fn main() -> int { return 0; }