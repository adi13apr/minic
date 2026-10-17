// RUN: %minic --dump-ast %s | FileCheck %s

fn f() -> int {
  return 1 + 2 * 3;
  return 10 - 3 - 2;
  return 8 / 4 / 2;
  return (1 + 2) * 3;
  return -a * b;
  return - -a;
  return !a == b;
  return a < b == c < d;
  return a || b && c;
  return a && b || c && d;
  return a + b < c * d && e != f;
  return f(1, g(2 + 3), h());
}
// CHECK:      Return (+ 1 (* 2 3))
// CHECK-NEXT: Return (- (- 10 3) 2)
// CHECK-NEXT: Return (/ (/ 8 4) 2)
// CHECK-NEXT: Return (* (+ 1 2) 3)
// CHECK-NEXT: Return (* (- a) b)
// CHECK-NEXT: Return (- (- a))
// CHECK-NEXT: Return (== (! a) b)
// CHECK-NEXT: Return (== (< a b) (< c d))
// CHECK-NEXT: Return (|| a (&& b c))
// CHECK-NEXT: Return (|| (&& a b) (&& c d))
// CHECK-NEXT: Return (&& (< (+ a b) (* c d)) (!= e f))
// CHECK-NEXT: Return (call f 1 (call g (+ 2 3)) (call h))