//===- Diagnostic.cpp - minic error reporting -----------------------------===//

#include "minic/Diagnostic.h"

#include "llvm/Support/SourceMgr.h"

using namespace minic;

void DiagnosticEngine::error(llvm::SMLoc Loc, const llvm::Twine &Message) {
  SrcMgr.PrintMessage(Loc, llvm::SourceMgr::DK_Error, Message);
  ++NumErrors;
}