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

  ### Parser and AST

- **Hand-written recursive descent**, one method per rule in `docs/grammar.md`, as in
  Clang. No parser generator dependency, and full control over error messages.
- **Precedence climbing for binary expressions.** One function reads the precedence
  table instead of one function per precedence level. Parsing the right operand at
  `precedence + 1` makes every operator left-associative.
- **Two-token lookahead** (current and next token) to tell `x = ...;` apart from
  `x(...);`, since both start with an identifier.
- **AST ownership with `std::unique_ptr`**: each node owns its children, so the whole
  tree is freed when the `Program` is destroyed. Names are `StringRef`s into the source
  buffer, so the AST must not outlive the `SourceMgr`.
- **LLVM-style RTTI** (`classof` with `isa<>`, `cast<>`, `dyn_cast<>`) instead of
  `dynamic_cast`: faster, independent of C++ RTTI, and the idiom used throughout LLVM.
- **No node for parentheses**: the tree's shape already records the grouping.
- **Panic-mode error recovery**: after an error, skip to the next `;` (or past a nested
  `{ ... }`), or to the next `fn` at top level. A missing `;` is reported right after the
  previous token and the statement is kept, so one mistake does not hide others.
- **`--dump-ast` stops after parsing**, so parser tests do not depend on Sema.
  Expressions print as S-expressions, which makes precedence tests exact.

  ### Semantic analysis

- **Two passes**: first every function signature is recorded, then bodies are
  checked, so calls may refer to functions defined later and recursion works.
- **Scopes as a stack of `llvm::StringMap`s**, searched innermost first. Redeclaring a
  name in the same block is an error (with a note at the earlier declaration); an
  inner block may shadow an outer name. Parameters share the function body's scope.
- **An `Error` type stops cascades**: once an expression is reported as invalid, its
  type is `Error` and nothing that uses it reports again. Operators always have a known
  result type, so `(1 + true) * 2` reports one error, not two.
- **Sema annotates the AST** with each expression's type and each literal's value, so
  codegen never re-derives them.
- **Sema only runs on an AST without syntax errors**, so it never has to handle a
  partially parsed program.
- **`-2147483648` is accepted** even though `2147483648` alone does not fit in `int`: a
  literal directly under unary minus may be one larger than `INT32_MAX`.
- **Missing-return check is conservative**: a non-void function must return on every
  path through `if`/`else`; loops never count, not even `while (true)`. Deciding whether
  a loop terminates is impossible in general, so the check errs on the safe side.
- **`print(int)` is a builtin** checked like a normal call, and cannot be redefined.

  See [docs/grammar.md](docs/grammar.md) for the full grammar