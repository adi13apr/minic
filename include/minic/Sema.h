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

/// Reports every semantic error through Diags. Must only be run on an AST
/// that was parsed without syntax errors.
void checkProgram(Program &Prog, DiagnosticEngine &Diags);

} // namespace minic

#endif // MINIC_SEMA_H