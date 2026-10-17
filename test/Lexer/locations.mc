// RUN: %minic --dump-tokens %s | FileCheck %s

let x
// CHECK:      kw_let 'let' [[@LINE-1]]:1
// CHECK-NEXT: identifier 'x' [[@LINE-2]]:5
    y
// CHECK-NEXT: identifier 'y' [[@LINE-1]]:5