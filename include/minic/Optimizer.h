//===- Optimizer.h - Run LLVM's optimization pipeline -------------*- C++
//-*-===//

#ifndef MINIC_OPTIMIZER_H
#define MINIC_OPTIMIZER_H

namespace llvm {
class Module;
class TargetMachine;
} // namespace llvm

namespace minic {

/// Runs LLVM's standard pipeline for -O1, -O2 or -O3 on M. Level 0 leaves
/// the module unchanged, so -O0 output is exactly what codegen produced.
/// If TM is given, the passes use its cost model (TargetTransformInfo).
void optimizeModule(llvm::Module &M, unsigned OptLevel,
                    llvm::TargetMachine *TM = nullptr);

} // namespace minic

#endif // MINIC_OPTIMIZER_H