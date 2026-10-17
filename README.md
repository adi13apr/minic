# minic

A compiler for minic, a small C-like language, written in C++17 on LLVM 18. It has
a hand-written lexer and recursive-descent parser, semantic analysis with scopes and
type checking, and LLVM IR code generation; the IR is optimized with LLVM's standard
pass pipelines and linked into a native executable.

```
fn factorial(n: int) -> int {
  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

fn main() -> int {
  print(factorial(10));
  return 0;
}
```

## Building

Requires LLVM 18.1, clang-18, CMake 3.20+, Ninja, and lit. On Ubuntu 24.04:

```
sudo apt install llvm-18-dev llvm-18-tools clang-18 ninja-build \
  zlib1g-dev libzstd-dev libedit-dev libcurl4-openssl-dev
pipx install lit

cmake -S . -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18
ninja -C build check
```

CMake options: `-DMINIC_WERROR=ON` treats warnings as errors, and
`-DMINIC_SANITIZE=ON` builds with AddressSanitizer and UndefinedBehaviorSanitizer.

## Usage
minic --emit-ptx -O2 device.mc               # compile for NVIDIA GPUs, print PTX
minic --emit-ptx --gpu-arch=sm_80 device.mc  # for a specific GPU architecture
```
minic program.mc -o program        # compile and link a native executable
minic -O2 program.mc -o program    # optimize with LLVM's -O2 pipeline
minic --emit-llvm program.mc       # print the LLVM IR
minic --dump-ast program.mc        # print the AST
minic --dump-tokens program.mc     # print the tokens
```

## Language Reference

- Types: `int` (32-bit signed, wraps on overflow) and `bool`. No implicit conversions.
- Functions: `fn name(a: int, b: bool) -> int { ... }`; omit `-> type` for no return
  value. A program needs `fn main() -> int`, whose result is the exit code.
- Statements: `let x: int = expr;`, `x = expr;`, `if (...) { } else if (...) { } else { }`,
  `while (...) { }`, `return expr;`, `{ ... }`, and function calls.
- Operators: `* /`, `+ -`, `< <= > >=`, `== !=`, `&&`, `||` (lowest), all
  left-associative, plus unary `-` and `!`. `&&` and `||` short-circuit. Division by
  zero and `INT_MIN / -1` trap at run time.
- Builtin: `print(int)` prints its argument on its own line.

The full grammar is in [docs/grammar.md](docs/grammar.md); runnable programs are in
[examples/](examples/).

## Architecture

```
source ─► Lexer ─► Parser ─► AST ─► Sema ─► CodeGen ─► LLVM IR ─► optimizer ─► clang ─► executable
                                                                  (-O1..-O3)     ▲
                                                                runtime library ─┘
```

| Stage     | Files                                    | Responsibility                                  |
|-----------|------------------------------------------|-------------------------------------------------|
| Lexer     | `Token.h`, `TokenKinds.def`, `Lexer.cpp` | Source text to tokens, with locations           |
| Parser    | `Parser.cpp`, `AST.h`                    | Tokens to AST, with error recovery              |
| Sema      | `Sema.cpp`                               | Name resolution, type checking, return checks   |
| CodeGen   | `CodeGen.cpp`                            | AST to verified LLVM IR                         |
| Optimizer | `Optimizer.cpp`                          | LLVM's standard new-pass-manager pipelines      |
| Driver    | `tools/minic/main.cpp`                   | Command line, stage order, linking with clang   |
| Runtime   | `runtime/runtime.c`                      | `minic_print`, linked into every program        |

Errors from every stage go through `DiagnosticEngine`, built on `llvm::SourceMgr`, and
print as `file:line:col: error: message` with the source line and a caret.

## Design Decisions

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

### Code generation

- **Locals live in stack slots (`alloca`) in the entry block**, read with `load` and
  written with `store`, as Clang does. LLVM's `mem2reg` pass then builds SSA form, so the
  frontend never computes phi nodes for variables. Allocas must be in the entry block
  for `mem2reg` to promote them, wherever the variable is declared.
- **Parameters are copied into slots** (`%x.addr`) because SSA arguments cannot be
  reassigned; `mem2reg` removes the copies.
- **Wrapping integer arithmetic**: no `nsw`/`nuw` flags, so overflow is defined
  (two's complement) instead of producing poison.
- **Division is guarded**: `sdiv` is undefined behavior in LLVM for division by zero and
  `INT_MIN / -1`, so both branch to a per-function block that calls `llvm.trap`. A
  constant divisor other than 0 and -1 skips the guard.
- **Short-circuit `&&` and `||`** use a conditional branch and a `phi`. The phi's
  incoming block is wherever the right operand *ended*, since a nested `&&`/`||` moves
  the insertion point to a new block.
- **Only `main` has external linkage**; every other function is `internal`, so the
  optimizer can inline it and delete the original.
- **Every block ends in exactly one terminator**: statements after a `return` are not
  emitted, an `if.end` that nothing jumps to is deleted, and a non-void function whose
  last block could fall through (impossible after Sema) ends in `unreachable`.
- **The module is always verified** with `llvm::verifyModule`; invalid IR is treated as a
  compiler bug and aborts.
- **`print` lowers to a call to `minic_print`**, provided by a small C runtime.

  See [docs/grammar.md](docs/grammar.md) for the full grammar

### Driver

- **Links by invoking clang** on the generated IR plus the runtime library, rather than
  embedding LLVM's backend and a linker. This keeps the compiler small and reuses
  clang's knowledge of the platform's linker and C library.
- **`-O1` to `-O3` use LLVM's standard pipelines** built by `PassBuilder` (the new pass
  manager). `-O0` leaves the IR exactly as codegen produced it, so codegen tests are
  stable.
- **The runtime is built without sanitizers**, even in a sanitizer build, because
  compiled programs are linked without sanitizer runtimes.
- **`minic_print` flushes stdout** after every call, so output printed before a
  division trap is not lost.


### GPU (PTX) output

- **`--emit-ptx` uses LLVM's NVPTX backend in-process**: a `TargetMachine` for
  `nvptx64-nvidia-cuda` and the chosen `sm_XX` architecture runs instruction selection
  and prints PTX, NVIDIA's virtual ISA. `ptxas` or the GPU driver turns PTX into SASS.
- **IR is generated for the GPU from the start**: the module gets the NVPTX triple and
  data layout before optimization, and the optimizer gets the `TargetMachine`, so passes
  use the GPU's cost model (`TargetTransformInfo`) instead of the host's.
- **Every function is a device function (`.func`) with external linkage**, so each one
  appears in the PTX. Kernels (`.entry`) are not supported: a kernel communicates only
  through memory, which needs pointer types that minic does not have yet.
- **`print` is rejected for the GPU**, and `main` is not required. CUDA implements
  device-side `printf` by lowering it to `vprintf`; doing the same is future work.
- **The NVPTX backend is optional**: CMake detects it from `LLVM_TARGETS_TO_BUILD`, and
  GPU tests are marked `REQUIRES: nvptx` so lit skips them when it is missing.
- **An unknown `--gpu-arch` is an error**, rather than LLVM's default of warning and
  silently using a fallback architecture.


## Testing

All tests use lit and FileCheck and run with `ninja -C build check`.

| Folder         | What it checks                                                          |
|----------------|-------------------------------------------------------------------------|
| `test/Driver`  | Command-line handling                                                   |
| `test/Lexer`   | Every token kind, locations, and lexical errors                         |
| `test/Parser`  | AST shape, precedence, syntax errors, and error recovery                |
| `test/Sema`    | Every semantic error, with exact `file:line:col` locations              |
| `test/CodeGen` | The generated IR, at `-O0` and `-O2`                                    |
| `test/E2E`     | Compiled programs run and print the right output, at `-O0` and `-O2`    |
| `test/PTX`     | PTX output for NVIDIA GPUs (skipped without the NVPTX backend)          |

CI runs three jobs on every push: a Release build with `-Werror`, a Debug build with
ASan and UBSan, and a clang-format check.

## Limitations

- Only `int` and `bool`: no arrays, strings, global variables, or modulo operator.
- The missing-return check is conservative: a `while (true)` loop never counts as
  returning.
- The runtime library's path is fixed at build time (override with `--runtime=<path>`).
- The optimizer runs without the target's data layout; clang applies it when linking.
- No debug information.
- GPU output has device functions only: kernels need pointer types, and device-side
  `print` would need lowering to CUDA's `vprintf`.

















