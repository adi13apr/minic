//===- CodeGen.h - Lower a minic AST to LLVM IR -------------------*- C++
//-*-===//
//
// Turns a program that passed Sema into an LLVM module. Every local variable
// gets a stack slot (alloca) in the function's entry block; LLVM's mem2reg
// pass later turns those slots into SSA registers.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_CODEGEN_H
#define MINIC_CODEGEN_H

#include "llvm/ADT/StringRef.h"

#include <memory>

namespace llvm {
class LLVMContext;
class Module;
} // namespace llvm

namespace minic {

class Program;

/// Generates IR for Prog and checks it with llvm::verifyModule. Invalid IR
/// is a compiler bug, so it aborts with a fatal error rather than returning.
std::unique_ptr<llvm::Module> generateIR(const Program &Prog,
                                         llvm::LLVMContext &Ctx,
                                         llvm::StringRef ModuleName);

} // namespace minic

#endif // MINIC_CODEGEN_H