//===- main.cpp - minic compiler driver ----------------------------------===//
//
// Entry point for the minic compiler: parses the command line and reads the
// input file. Later phases add lexing, parsing, and code generation.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>

using namespace llvm;

static cl::OptionCategory MinicCategory("minic options");

static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<input file>"),
                                          cl::cat(MinicCategory));

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);

  cl::HideUnrelatedOptions(MinicCategory);
  cl::ParseCommandLineOptions(argc, argv,
                              "minic - compiler for the minic language\n");

  ErrorOr<std::unique_ptr<MemoryBuffer>> BufferOrErr =
      MemoryBuffer::getFile(InputFilename);
  if (std::error_code EC = BufferOrErr.getError()) {
    WithColor::error(errs(), "minic")
        << "could not open '" << InputFilename << "': " << EC.message() << '\n';
    return 1;
  }

  // Phase 1: lex the contents of *BufferOrErr.
  return 0;
}