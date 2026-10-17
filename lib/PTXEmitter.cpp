//===- PTXEmitter.cpp - Compile LLVM IR to PTX for NVIDIA GPUs ------------===//

#include "minic/PTXEmitter.h"

#include "llvm/ADT/Twine.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include <optional>

std::unique_ptr<llvm::TargetMachine>
minic::createNVPTXTargetMachine(llvm::StringRef GPUArch, unsigned OptLevel,
                                std::string &Error) {
  // Register only the NVPTX backend, not every target LLVM was built with.
  LLVMInitializeNVPTXTargetInfo();
  LLVMInitializeNVPTXTarget();
  LLVMInitializeNVPTXTargetMC();
  LLVMInitializeNVPTXAsmPrinter();

  const llvm::Target *Target =
      llvm::TargetRegistry::lookupTarget(NVPTXTriple, Error);
  if (!Target)
    return nullptr;

  // Reject a misspelled architecture up front. Otherwise LLVM only prints a
  // warning and silently falls back to a default architecture.
  std::unique_ptr<llvm::MCSubtargetInfo> STI(
      Target->createMCSubtargetInfo(NVPTXTriple, "", ""));
  if (!STI || !STI->isCPUStringValid(GPUArch)) {
    Error = ("unknown GPU architecture '" + GPUArch + "'").str();
    return nullptr;
  }

  llvm::CodeGenOptLevel Level = OptLevel == 0   ? llvm::CodeGenOptLevel::None
                                : OptLevel == 1 ? llvm::CodeGenOptLevel::Less
                                : OptLevel == 2
                                    ? llvm::CodeGenOptLevel::Default
                                    : llvm::CodeGenOptLevel::Aggressive;

  return std::unique_ptr<llvm::TargetMachine>(Target->createTargetMachine(
      NVPTXTriple, GPUArch, /*Features=*/"", llvm::TargetOptions(),
      /*RM=*/std::nullopt, /*CM=*/std::nullopt, Level));
}

bool minic::emitPTX(llvm::Module &M, llvm::TargetMachine &TM,
                    llvm::raw_pwrite_stream &OS) {
  // In LLVM 18 the backend (instruction selection and assembly printing)
  // still runs on the legacy pass manager.
  llvm::legacy::PassManager PM;
  if (TM.addPassesToEmitFile(PM, OS, /*DwoOut=*/nullptr,
                             llvm::CodeGenFileType::AssemblyFile))
    return false; // The target cannot emit this kind of file.
  PM.run(M);
  return true;
}