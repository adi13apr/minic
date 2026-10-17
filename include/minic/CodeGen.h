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
#include <string>

namespace llvm {
class LLVMContext;
class Module;
} // namespace llvm

namespace minic {

class Program;

struct CodeGenOptions {
  /// Target triple for the module; empty means the machine minic runs on.
  std::string TargetTriple;
  /// Data layout for the module; empty leaves it for clang to fill in when
  /// linking.
  std::string DataLayout;
  /// Give every function external linkage, not only main. Used for GPU code,
  /// where each function is a device function meant to be called from
  /// other code.
  bool ExportAllFunctions = false;
};

/// Generates IR for Prog and checks it with llvm::verifyModule. Invalid IR
/// is a compiler bug, so it aborts with a fatal error rather than returning.
std::unique_ptr<llvm::Module>
generateIR(const Program &Prog, llvm::LLVMContext &Ctx,
           llvm::StringRef ModuleName,
           const CodeGenOptions &Opts = CodeGenOptions());

} // namespace minic

#endif // MINIC_CODEGEN_H