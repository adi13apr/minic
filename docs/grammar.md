# minic Grammar

This is the complete grammar of minic in EBNF. `IDENT` and `INT_LIT` are the
`identifier` and `integer_literal` tokens; quoted strings are punctuators or
keywords. Comments start with `//` and run to the end of the line.

## Declarations

```
program      ::= function* EOF
function     ::= 'fn' IDENT '(' param_list? ')' ( '->' type )? block
param_list   ::= param ( ',' param )*
param        ::= IDENT ':' type
type         ::= 'int' | 'bool'
```

A function with no `->` returns nothing (void). `main` must be declared as
`fn main() -> int`; its return value is the program's exit code.

## Statements

```
block        ::= '{' statement* '}'
statement    ::= let_stmt
               | assign_stmt
               | if_stmt
               | while_stmt
               | return_stmt
               | block
               | expr_stmt
let_stmt     ::= 'let' IDENT ':' type '=' expr ';'
assign_stmt  ::= IDENT '=' expr ';'
if_stmt      ::= 'if' '(' expr ')' block ( 'else' ( if_stmt | block ) )?
while_stmt   ::= 'while' '(' expr ')' block
return_stmt  ::= 'return' expr? ';'
expr_stmt    ::= expr ';'
```

`assign_stmt` and `expr_stmt` both can start with `IDENT`; the parser looks
one token past the identifier to choose (`=` means assignment).

## Expressions

```
expr         ::= unary ( binary_op unary )*     // resolved by precedence
unary        ::= ( '-' | '!' ) unary
               | primary
primary      ::= INT_LIT
               | 'true' | 'false'
               | IDENT
               | IDENT '(' arg_list? ')'
               | '(' expr ')'
arg_list     ::= expr ( ',' expr )*
```

Binary operators, from highest to lowest precedence. All are left-associative.

| Precedence | Operators            |
|-----------:|----------------------|
|          6 | `*` `/`              |
|          5 | `+` `-`              |
|          4 | `<` `<=` `>` `>=`    |
|          3 | `==` `!=`            |
|          2 | `&&`                 |
|          1 | `\|\|`               |

Unary `-` and `!` bind tighter than every binary operator.

## Rules enforced after parsing (Sema)

- No implicit conversion between `int` and `bool`.
- Arithmetic and `<` `<=` `>` `>=` need `int` operands; `&&` `||` `!` need
  `bool`; `==` `!=` need two operands of the same type.
- `if` and `while` conditions must be `bool`.
- An expression statement must be a function call.
- A name can be declared once per block; inner blocks may shadow outer names.
- Integer literals must fit in a signed 32-bit integer.
- `print(int)` is a builtin function, not a keyword.