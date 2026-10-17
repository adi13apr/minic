// RUN: %minic --dump-tokens %s | FileCheck %s

fn let return if else while int bool true false
// CHECK:      kw_fn 'fn'
// CHECK-NEXT: kw_let 'let'
// CHECK-NEXT: kw_return 'return'
// CHECK-NEXT: kw_if 'if'
// CHECK-NEXT: kw_else 'else'
// CHECK-NEXT: kw_while 'while'
// CHECK-NEXT: kw_int 'int'
// CHECK-NEXT: kw_bool 'bool'
// CHECK-NEXT: kw_true 'true'
// CHECK-NEXT: kw_false 'false'

// Keywords are case-sensitive and must be whole words. 'print' is a
// builtin function resolved by Sema, not a keyword.
If fnx _let while2 print x_1 _
// CHECK-NEXT: identifier 'If'
// CHECK-NEXT: identifier 'fnx'
// CHECK-NEXT: identifier '_let'
// CHECK-NEXT: identifier 'while2'
// CHECK-NEXT: identifier 'print'
// CHECK-NEXT: identifier 'x_1'
// CHECK-NEXT: identifier '_'
// CHECK-NEXT: eof