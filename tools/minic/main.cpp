//===- main.cpp - minic compiler driver ----------------------------------===//
//
// Entry point for the minic compiler. Runs every stage on the input file and,
// by default, links the result into a native executable by invoking clang on
// the generated IR together with the minic runtime library. With --emit-ptx
// it compiles for NVIDIA GPUs instead, using LLVM's NVPTX backend.
//
//===----------------------------------------------------------------------===//

#include "minic/ASTDumper.h"
#include "minic/CodeGen.h"
#include "minic/Diagnostic.h"
#include "minic/Lexer.h"
#include "minic/Optimizer.h"
#include "minic/Parser.h"
#include "minic/Sema.h"
#include "minic/Token.h"

#ifdef MINIC_HAS_NVPTX
#include "minic/PTXEmitter.h"
#include "llvm/Target/TargetMachine.h"
#endif

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#ifndef MINIC_RUNTIME_PATH
#error "MINIC_RUNTIME_PATH must be defined by the build system"
#endif

using namespace llvm;

static cl::OptionCategory MinicCategory("minic options");

static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<input file>"),
                                          cl::cat(MinicCategory));

static cl::opt<std::string> OutputFilename(
    "o",
    cl::desc("Write output to <file> (default: a.out, or stdout with "
             "--emit-llvm or --emit-ptx)"),
    cl::value_desc("file"), cl::cat(MinicCategory));

static cl::opt<unsigned>
    OptLevel("O", cl::desc("Optimization level: -O0 (default) to -O3"),
             cl::Prefix, cl::init(0), cl::cat(MinicCategory));

static cl::opt<bool>
    DumpTokens("dump-tokens",
               cl::desc("Print each token with its location, then stop"),
               cl::cat(MinicCategory));

static cl::opt<bool>
    DumpAST("dump-ast",
            cl::desc("Print the AST after parsing (no semantic checks)"),
            cl::cat(MinicCategory));

static cl::opt<bool>
    EmitLLVM("emit-llvm",
             cl::desc("Write LLVM IR instead of linking an executable"),
             cl::cat(MinicCategory));

static cl::opt<bool>
    EmitPTX("emit-ptx",
            cl::desc("Compile for NVIDIA GPUs and write PTX assembly"),
            cl::cat(MinicCategory));

static cl::opt<std::string>
    GPUArch("gpu-arch",
            cl::desc("GPU architecture for --emit-ptx (default: sm_70)"),
            cl::value_desc("sm_XX"), cl::init("sm_70"), cl::cat(MinicCategory));

static cl::opt<std::string>
    ClangPath("clang",
              cl::desc("Path to the clang used for linking (default: "
                       "clang-18 or clang from PATH)"),
              cl::value_desc("path"), cl::cat(MinicCategory));

static cl::opt<std::string>
    RuntimePath("runtime", cl::desc("Path to the minic runtime library"),
                cl::value_desc("path"), cl::init(MINIC_RUNTIME_PATH),
                cl::cat(MinicCategory));

static int reportError(const Twine &Message) {
  WithColor::error(errs(), "minic") << Message << '\n';
  return 1;
}

static void dumpToken(const minic::Token &Tok, const SourceMgr &SrcMgr,
                      unsigned BufferID, raw_ostream &OS) {
  auto [Line, Column] = SrcMgr.getLineAndColumn(Tok.getLocation(), BufferID);
  OS << minic::tok::getTokenName(Tok.getKind()) << " '" << Tok.getText() << "' "
     << Line << ':' << Column << '\n';
}

/// Writes M as textual IR to Path ("-" means stdout).
static bool writeIR(const Module &M, StringRef Path) {
  std::error_code EC;
  ToolOutputFile Out(Path, EC, sys::fs::OF_Text);
  if (EC) {
    reportError("could not open '" + Path + "': " + EC.message());
    return false;
  }
  M.print(Out.os(), nullptr);
  Out.keep(); // Without keep(), the file is deleted when Out is destroyed.
  return true;
}

static std::optional<std::string> findClang() {
  if (!ClangPath.empty())
    return std::string(ClangPath);
  for (StringRef Name : {"clang-18", "clang"})
    if (ErrorOr<std::string> Path = sys::findProgramByName(Name))
      return *Path;
  return std::nullopt;
}

/// Writes M to a temporary .ll file and runs clang on it together with the
/// runtime library, producing a native executable at OutputPath.
static bool linkExecutable(const Module &M, StringRef OutputPath,
                           unsigned Level) {
  std::optional<std::string> Clang = findClang();
  if (!Clang) {
    reportError("could not find clang-18 or clang in PATH; use --clang=<path>");
    return false;
  }

  SmallString<128> IRPath;
  if (std::error_code EC =
          sys::fs::createTemporaryFile("minic", "ll", IRPath)) {
    reportError("could not create a temporary file: " + EC.message());
    return false;
  }
  FileRemover RemoveIR(IRPath); // Deletes the file when this function returns.
  if (!writeIR(M, IRPath))
    return false;

  std::string OptFlag = "-O" + std::to_string(Level);
  SmallVector<StringRef, 8> Args = {*Clang,      OptFlag, IRPath,
                                    RuntimePath, "-o",    OutputPath};

  std::string ErrMsg;
  int Result = sys::ExecuteAndWait(*Clang, Args, /*Env=*/std::nullopt,
                                   /*Redirects=*/{}, /*SecondsToWait=*/0,
                                   /*MemoryLimit=*/0, &ErrMsg);
  if (Result == 0)
    return true;

  // If clang ran but failed, it has already printed its own errors.
  if (ErrMsg.empty())
    reportError("linking with '" + *Clang + "' failed");
  else
    reportError("could not run '" + *Clang + "': " + ErrMsg);
  return false;
}

/// Compiles Prog for an NVIDIA GPU and writes PTX (or, with --emit-llvm, the
/// GPU-targeted IR) to OutputPath.
static int compileForGPU(const minic::Program &Prog, LLVMContext &Ctx,
                         StringRef OutputPath, unsigned Level) {
#ifdef MINIC_HAS_NVPTX
  std::string Error;
  std::unique_ptr<TargetMachine> TM =
      minic::createNVPTXTargetMachine(GPUArch, Level, Error);
  if (!TM)
    return reportError(Error);

  // Generate the IR for the GPU from the start, so the optimizer sees the
  // GPU's data layout rather than the host's.
  minic::CodeGenOptions Opts;
  Opts.TargetTriple = TM->getTargetTriple().str();
  Opts.DataLayout = TM->createDataLayout().getStringRepresentation();
  Opts.ExportAllFunctions = true;

  std::unique_ptr<Module> M = minic::generateIR(Prog, Ctx, InputFilename, Opts);
  minic::optimizeModule(*M, Level, TM.get());

  if (EmitLLVM)
    return writeIR(*M, OutputPath) ? 0 : 1;

  std::error_code EC;
  ToolOutputFile Out(OutputPath, EC, sys::fs::OF_Text);
  if (EC)
    return reportError("could not open '" + OutputPath + "': " + EC.message());
  if (!minic::emitPTX(*M, *TM, Out.os()))
    return reportError("the NVPTX target cannot emit PTX assembly");
  Out.keep();
  return 0;
#else
  (void)Prog;
  (void)Ctx;
  (void)OutputPath;
  (void)Level;
  return reportError("this minic was built without LLVM's NVPTX target, so "
                     "--emit-ptx is unavailable");
#endif
}

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);

  cl::HideUnrelatedOptions(MinicCategory);
  cl::ParseCommandLineOptions(argc, argv,
                              "minic - compiler for the minic language\n");

  unsigned Level = OptLevel;
  if (Level > 3)
    return reportError("invalid optimization level -O" + std::to_string(Level) +
                       "; use -O0 to -O3");

  ErrorOr<std::unique_ptr<MemoryBuffer>> BufferOrErr =
      MemoryBuffer::getFile(InputFilename);
  if (std::error_code EC = BufferOrErr.getError())
    return reportError("could not open '" + InputFilename +
                       "': " + EC.message());

  SourceMgr SrcMgr;
  unsigned BufferID =
      SrcMgr.AddNewSourceBuffer(std::move(*BufferOrErr), SMLoc());
  minic::DiagnosticEngine Diags(SrcMgr);
  minic::Lexer Lex(SrcMgr, BufferID, Diags);

  if (DumpTokens) {
    while (true) {
      minic::Token Tok = Lex.lex();
      dumpToken(Tok, SrcMgr, BufferID, outs());
      if (Tok.is(minic::tok::eof))
        break;
    }
    return Diags.hasErrors() ? 1 : 0;
  }

  minic::Parser P(Lex, Diags);
  std::unique_ptr<minic::Program> Prog = P.parseProgram();

  if (DumpAST) {
    minic::dumpAST(*Prog, outs());
    return Diags.hasErrors() ? 1 : 0;
  }

  // Sema assumes a well-formed AST, so it only runs if parsing succeeded.
  if (Diags.hasErrors())
    return 1;

  // GPU code has no main and no runtime library for print.
  minic::SemaOptions SemaOpts;
  if (EmitPTX) {
    SemaOpts.RequireMain = false;
    SemaOpts.AllowPrint = false;
  }
  minic::checkProgram(*Prog, Diags, SemaOpts);
  if (Diags.hasErrors())
    return 1;

  std::string Output = OutputFilename;
  if (Output.empty())
    Output = EmitLLVM || EmitPTX ? "-" : "a.out";

  // The context must outlive every module, so it is declared first: locals
  // are destroyed in reverse order of declaration.
  LLVMContext Ctx;

  if (EmitPTX)
    return compileForGPU(*Prog, Ctx, Output, Level);

  std::unique_ptr<Module> M = minic::generateIR(*Prog, Ctx, InputFilename);
  minic::optimizeModule(*M, Level);

  if (EmitLLVM)
    return writeIR(*M, Output) ? 0 : 1;
  return linkExecutable(*M, Output, Level) ? 0 : 1;
}