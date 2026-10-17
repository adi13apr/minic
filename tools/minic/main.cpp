//===- main.cpp - minic compiler driver ----------------------------------===//
//
// Entry point for the minic compiler: parses the command line, reads the
// input file, and runs the compiler stages on it.
//
//===----------------------------------------------------------------------===//

#include "minic/Diagnostic.h"
#include "minic/Lexer.h"
#include "minic/Token.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <utility>

using namespace llvm;

static cl::OptionCategory MinicCategory("minic options");

static cl::opt<std::string> InputFilename(cl::Positional, cl::Required,
                                          cl::desc("<input file>"),
                                          cl::cat(MinicCategory));

static cl::opt<bool> DumpTokens("dump-tokens",
                                cl::desc("Print each token with its location"),
                                cl::cat(MinicCategory));

static void dumpToken(const minic::Token &Tok, const SourceMgr &SrcMgr,
                      unsigned BufferID, raw_ostream &OS) {
  auto [Line, Column] = SrcMgr.getLineAndColumn(Tok.getLocation(), BufferID);
  OS << minic::tok::getTokenName(Tok.getKind()) << " '" << Tok.getText() << "' "
     << Line << ':' << Column << '\n';
}

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

  SourceMgr SrcMgr;
  unsigned BufferID =
      SrcMgr.AddNewSourceBuffer(std::move(*BufferOrErr), SMLoc());
  minic::DiagnosticEngine Diags(SrcMgr);
  minic::Lexer Lex(SrcMgr, BufferID, Diags);

  // Until the parser exists, lex the whole file so that every lexical error
  // is reported.
  while (true) {
    minic::Token Tok = Lex.lex();
    if (DumpTokens)
      dumpToken(Tok, SrcMgr, BufferID, outs());
    if (Tok.is(minic::tok::eof))
      break;
  }

  return Diags.hasErrors() ? 1 : 0;
}