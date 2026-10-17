### Lexer

- **Token kinds in one X-macro file (`TokenKinds.def`)**, as in Clang. The enum, the
  name table, and the keyword lookup are all generated from it, so adding a token is
  a one-line change.
- **Tokens store a `StringRef` into the source buffer**, not a copy. The location is
  derived from that pointer, and `llvm::SourceMgr` turns it into line and column only
  when needed. The buffer is owned by `SourceMgr`, which outlives all tokens.
- **On-demand lexing** (`lex()` returns one token at a time), as in Clang, instead of
  building a vector of all tokens up front.
- **Maximal munch**: the lexer always takes the longest operator (`<==` is `<=` `=`).
- **Error recovery**: a bad character is reported, becomes a `tok::unknown` token, and
  lexing continues, so one run reports every lexical error.
- **Integer range is checked in Sema, not the lexer**, because only Sema knows the
  context (for example, unary minus applied to a literal).
- **`print` is not a keyword**: it is an ordinary identifier that Sema resolves to a
  builtin function, so the grammar stays smaller.
- **Diagnostics use `llvm::SourceMgr`**, giving `file:line:col: error:` output with the
  source line and a caret, matching Clang.