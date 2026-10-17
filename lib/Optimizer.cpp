//===- Optimizer.cpp - Run LLVM's optimization pipeline -------------------===//

#include "minic/Optimizer.h"

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"

void minic::optimizeModule(llvm::Module &M, unsigned OptLevel,
                           llvm::TargetMachine *TM) {
  if (OptLevel == 0)
    return;

  llvm::OptimizationLevel Level = OptLevel == 1   ? llvm::OptimizationLevel::O1
                                  : OptLevel == 2 ? llvm::OptimizationLevel::O2
                                                  : llvm::OptimizationLevel::O3;

  // The new pass manager has one analysis manager per kind of IR unit. They
  // refer to each other, so they must be declared in this order to be
  // destroyed in the reverse order safely.
  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;

  // With a TargetMachine, passes such as the inliner, loop unroller and
  // vectorizer ask the target what instructions cost, instead of guessing.
  llvm::PassBuilder PB(TM);
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(Level);
  MPM.run(M, MAM);
}