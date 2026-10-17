//===- PTXEmitter.h - Compile LLVM IR to PTX for NVIDIA GPUs ------*- C++
//-*-===//
//
// Uses LLVM's NVPTX backend to turn a module into PTX, NVIDIA's virtual GPU
// instruction set. NVIDIA's ptxas (or the GPU driver) then compiles PTX into
// SASS, the machine code of a specific GPU.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_PTXEMITTER_H
#define MINIC_PTXEMITTER_H

#include "llvm/ADT/StringRef.h"

#include <memory>
#include <string>

namespace llvm {
class Module;
class TargetMachine;
class raw_pwrite_stream;
} // namespace llvm

namespace minic {

/// The target triple for 64-bit NVIDIA GPUs.
inline constexpr const char *NVPTXTriple = "nvptx64-nvidia-cuda";

/// Creates a TargetMachine for the given GPU architecture (e.g. "sm_70").
/// Returns null and sets Error if the target or architecture is unknown.
std::unique_ptr<llvm::TargetMachine>
createNVPTXTargetMachine(llvm::StringRef GPUArch, unsigned OptLevel,
                         std::string &Error);

/// Runs the NVPTX backend on M and writes PTX assembly to OS. Returns false
/// if the target cannot produce assembly output.
bool emitPTX(llvm::Module &M, llvm::TargetMachine &TM,
             llvm::raw_pwrite_stream &OS);

} // namespace minic

#endif // MINIC_PTXEMITTER_H