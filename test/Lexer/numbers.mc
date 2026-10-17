// RUN: %minic --dump-tokens %s | FileCheck %s

// The lexer does not check ranges: 2147483648 does not fit in i32, but
// that is reported by Sema, which knows the expected type.
0 42 007 2147483648
// CHECK:      integer_literal '0'
// CHECK-NEXT: integer_literal '42'
// CHECK-NEXT: integer_literal '007'
// CHECK-NEXT: integer_literal '2147483648'
// CHECK-NEXT: eof