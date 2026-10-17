//===- Diagnostic.h - minic error reporting -----------------------*- C++
//-*-===//
//
// Reports errors through llvm::SourceMgr, which prints them in the familiar
// "file:line:col: error: message" format with the source line and a caret,
// and counts them so the driver can choose its exit code.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_DIAGNOSTIC_H
#define MINIC_DIAGNOSTIC_H

#include "llvm/ADT/Twine.h"
#include "llvm/Support/SMLoc.h"

namespace llvm {
class SourceMgr;
} // namespace llvm

namespace minic {

class DiagnosticEngine {
public:
  explicit DiagnosticEngine(const llvm::SourceMgr &SrcMgr) : SrcMgr(SrcMgr) {}

  void error(llvm::SMLoc Loc, const llvm::Twine &Message);

  unsigned getNumErrors() const { return NumErrors; }
  bool hasErrors() const { return NumErrors != 0; }

private:
  const llvm::SourceMgr &SrcMgr;
  unsigned NumErrors = 0;
};

} // namespace minic

#endif // MINIC_DIAGNOSTIC_H