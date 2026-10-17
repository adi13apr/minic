// RUN: %minic --dump-tokens %s | FileCheck %s

( ) { } ; : ,
// CHECK:      l_paren '('
// CHECK-NEXT: r_paren ')'
// CHECK-NEXT: l_brace '{'
// CHECK-NEXT: r_brace '}'
// CHECK-NEXT: semi ';'
// CHECK-NEXT: colon ':'
// CHECK-NEXT: comma ','

+ - * / = !
// CHECK-NEXT: plus '+'
// CHECK-NEXT: minus '-'
// CHECK-NEXT: star '*'
// CHECK-NEXT: slash '/'
// CHECK-NEXT: equal '='
// CHECK-NEXT: exclaim '!'

== != < <= > >= -> && ||
// CHECK-NEXT: equalequal '=='
// CHECK-NEXT: exclaimequal '!='
// CHECK-NEXT: less '<'
// CHECK-NEXT: lessequal '<='
// CHECK-NEXT: greater '>'
// CHECK-NEXT: greaterequal '>='
// CHECK-NEXT: arrow '->'
// CHECK-NEXT: ampamp '&&'
// CHECK-NEXT: pipepipe '||'
// CHECK-NEXT: eof