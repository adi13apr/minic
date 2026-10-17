//===- ASTDumper.h - Print a minic AST ----------------------------*- C++
//-*-===//
//
// Prints declarations and statements as an indented tree, and each expression
// on one line as an S-expression, e.g. "1 + 2 * 3" prints as "(+ 1 (* 2 3))".
// The S-expression form makes precedence and associativity easy to test.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_ASTDUMPER_H
#define MINIC_ASTDUMPER_H

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace minic {

class Program;

void dumpAST(const Program &Prog, llvm::raw_ostream &OS);

} // namespace minic

#endif // MINIC_ASTDUMPER_H