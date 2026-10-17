//===- Sema.h - minic semantic analysis ---------------------------*- C++
//-*-===//
//
// Checks names and types after parsing and annotates the AST for codegen:
// every Expr gets its type, and every integer literal gets its value.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_SEMA_H
#define MINIC_SEMA_H

namespace minic {

class DiagnosticEngine;
class Program;

struct SemaOptions {
  /// Require 'fn main() -> int'. Off for GPU code, which has no main.
  bool RequireMain = true;
  /// Allow the print builtin. Off for GPU code, which has no runtime library.
  bool AllowPrint = true;
};

/// Reports every semantic error through Diags. Must only be run on an AST
/// that was parsed without syntax errors.
void checkProgram(Program &Prog, DiagnosticEngine &Diags,
                  const SemaOptions &Opts = SemaOptions());

} // namespace minic

#endif // MINIC_SEMA_H