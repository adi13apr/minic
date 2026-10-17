// RUN: %minic --dump-tokens %s | FileCheck %s

// A whole-line comment produces no tokens.
x / y // a trailing comment
// CHECK:      identifier 'x'
// CHECK-NEXT: slash '/'
// CHECK-NEXT: identifier 'y'
//// Extra slashes are still a comment.
// CHECK-NEXT: eof