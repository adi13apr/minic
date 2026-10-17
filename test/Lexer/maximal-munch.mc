// RUN: %minic --dump-tokens %s | FileCheck %s

// The lexer always takes the longest operator that matches.
<== ->- !== a-b
// CHECK:      lessequal '<='
// CHECK-NEXT: equal '='
// CHECK-NEXT: arrow '->'
// CHECK-NEXT: minus '-'
// CHECK-NEXT: exclaimequal '!='
// CHECK-NEXT: equal '='
// CHECK-NEXT: identifier 'a'
// CHECK-NEXT: minus '-'
// CHECK-NEXT: identifier 'b'
// CHECK-NEXT: eof