//===- CodeGen.cpp - Lower a minic AST to LLVM IR -------------------------===//

#include "minic/CodeGen.h"
#include "minic/AST.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"

#include <cstdint>
#include <vector>

using namespace minic;

namespace {

using Scope = llvm::StringMap<llvm::AllocaInst *>;

/// Pushes a scope on construction and pops it on destruction.
class ScopeGuard {
public:
  explicit ScopeGuard(std::vector<Scope> &Scopes) : Scopes(Scopes) {
    Scopes.emplace_back();
  }
  ~ScopeGuard() { Scopes.pop_back(); }
  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;

private:
  std::vector<Scope> &Scopes;
};

class CodeGenerator {
public:
  CodeGenerator(llvm::LLVMContext &Ctx, llvm::StringRef ModuleName)
      : Ctx(Ctx), M(std::make_unique<llvm::Module>(ModuleName, Ctx)),
        Builder(Ctx) {}

  std::unique_ptr<llvm::Module> run(const Program &Prog);

private:
  llvm::Type *getLLVMType(Type T);
  void declareFunction(const FunctionDecl &Fn);
  void emitFunction(const FunctionDecl &Fn);

  void emitBlock(const BlockStmt &Block);
  void emitStmts(const BlockStmt &Block);
  void emitStmt(const Stmt &S);
  void emitLet(const LetStmt &Let);
  void emitIf(const IfStmt &If);
  void emitWhile(const WhileStmt &While);

  llvm::Value *emitExpr(const Expr &E);
  llvm::Value *emitUnary(const UnaryExpr &Unary);
  llvm::Value *emitBinary(const BinaryExpr &Binary);
  llvm::Value *emitShortCircuit(const BinaryExpr &Binary);
  llvm::Value *emitDivision(llvm::Value *LHS, llvm::Value *RHS);
  llvm::Value *emitCall(const CallExpr &Call);

  llvm::AllocaInst *createEntryAlloca(llvm::Type *Ty, const llvm::Twine &Name);
  llvm::AllocaInst *lookupVar(llvm::StringRef Name) const;
  void startBlock(llvm::BasicBlock *BB);
  bool hasTerminator() const;
  llvm::Function *getPrintFunction();
  llvm::BasicBlock *getTrapBlock();

  llvm::LLVMContext &Ctx;
  std::unique_ptr<llvm::Module> M;
  llvm::IRBuilder<> Builder;
  std::vector<Scope> Scopes;
  llvm::Function *CurFn = nullptr;
  /// One shared block per function that traps on invalid division.
  llvm::BasicBlock *TrapBB = nullptr;
};

} // namespace

//===----------------------------------------------------------------------===//
// Functions
//===----------------------------------------------------------------------===//

std::unique_ptr<llvm::Module> CodeGenerator::run(const Program &Prog) {
  // Declare every function first, so calls can refer to any of them.
  // Tag the module with the machine minic runs on, which is also the
  // machine clang will compile it for when linking.
  M->setTargetTriple(llvm::sys::getDefaultTargetTriple());
  for (const std::unique_ptr<FunctionDecl> &Fn : Prog.getFunctions())
    declareFunction(*Fn);
  for (const std::unique_ptr<FunctionDecl> &Fn : Prog.getFunctions())
    emitFunction(*Fn);

  if (llvm::verifyModule(*M, &llvm::errs()))
    llvm::report_fatal_error(
        "minic generated invalid LLVM IR; this is a compiler bug");
  return std::move(M);
}

llvm::Type *CodeGenerator::getLLVMType(Type T) {
  switch (T) {
  case Type::Int:
    return Builder.getInt32Ty();
  case Type::Bool:
    return Builder.getInt1Ty();
  case Type::Void:
    return Builder.getVoidTy();
  case Type::Unknown:
  case Type::Error:
    break;
  }
  llvm_unreachable("codegen requires a program that passed Sema");
}

void CodeGenerator::declareFunction(const FunctionDecl &Fn) {
  std::vector<llvm::Type *> ParamTypes;
  for (const ParamDecl &Param : Fn.getParams())
    ParamTypes.push_back(getLLVMType(Param.Ty));
  auto *FnTy = llvm::FunctionType::get(getLLVMType(Fn.getReturnType()),
                                       ParamTypes, /*isVarArg=*/false);

  // Only main is visible outside the module. Internal linkage lets the
  // optimizer inline the other functions and then delete them.
  auto Linkage = Fn.getName() == "main" ? llvm::Function::ExternalLinkage
                                        : llvm::Function::InternalLinkage;
  llvm::Function *F =
      llvm::Function::Create(FnTy, Linkage, Fn.getName(), M.get());
  for (auto [Arg, Param] : llvm::zip(F->args(), Fn.getParams()))
    Arg.setName(Param.Name);
}

void CodeGenerator::emitFunction(const FunctionDecl &Fn) {
  CurFn = M->getFunction(Fn.getName());
  TrapBB = nullptr;
  startBlock(llvm::BasicBlock::Create(Ctx, "entry"));

  // Parameters and the body's outermost block share one scope, as in Sema.
  ScopeGuard Guard(Scopes);

  // Arguments are SSA values and cannot be assigned to, so each one is
  // copied into its own stack slot. mem2reg removes these copies later.
  for (auto [Arg, Param] : llvm::zip(CurFn->args(), Fn.getParams())) {
    llvm::AllocaInst *Slot =
        createEntryAlloca(Arg.getType(), Param.Name + ".addr");
    Builder.CreateStore(&Arg, Slot);
    Scopes.back()[Param.Name] = Slot;
  }

  emitStmts(*Fn.getBody());

  // If control can fall off the end of the body, close the last block. Sema
  // guarantees that a non-void function returns on every path, so for one of
  // those, this block can never be reached.
  if (!hasTerminator()) {
    if (Fn.getReturnType() == Type::Void)
      Builder.CreateRetVoid();
    else
      Builder.CreateUnreachable();
  }

  if (TrapBB)
    CurFn->insert(CurFn->end(), TrapBB);
  CurFn = nullptr;
}

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

void CodeGenerator::emitBlock(const BlockStmt &Block) {
  ScopeGuard Guard(Scopes);
  emitStmts(Block);
}

void CodeGenerator::emitStmts(const BlockStmt &Block) {
  for (const std::unique_ptr<Stmt> &S : Block.getBody()) {
    // Statements after a return are unreachable, and the current block is
    // already closed, so there is nowhere valid to put their code.
    if (hasTerminator())
      return;
    emitStmt(*S);
  }
}

void CodeGenerator::emitStmt(const Stmt &S) {
  switch (S.getKind()) {
  case Stmt::Kind::Block:
    emitBlock(llvm::cast<BlockStmt>(S));
    return;
  case Stmt::Kind::Let:
    emitLet(llvm::cast<LetStmt>(S));
    return;
  case Stmt::Kind::Assign: {
    const auto &Assign = llvm::cast<AssignStmt>(S);
    llvm::Value *Value = emitExpr(*Assign.getValue());
    Builder.CreateStore(Value, lookupVar(Assign.getName()));
    return;
  }
  case Stmt::Kind::If:
    emitIf(llvm::cast<IfStmt>(S));
    return;
  case Stmt::Kind::While:
    emitWhile(llvm::cast<WhileStmt>(S));
    return;
  case Stmt::Kind::Return:
    if (const Expr *Value = llvm::cast<ReturnStmt>(S).getValue())
      Builder.CreateRet(emitExpr(*Value));
    else
      Builder.CreateRetVoid();
    return;
  case Stmt::Kind::ExprStmt:
    emitExpr(*llvm::cast<ExprStmt>(S).getExpr());
    return;
  }
  llvm_unreachable("unknown statement kind");
}

void CodeGenerator::emitLet(const LetStmt &Let) {
  // Emit the initializer before declaring the name, matching Sema: in
  // "let x: int = x;" the right-hand x is an outer variable.
  llvm::Value *Init = emitExpr(*Let.getInit());
  llvm::AllocaInst *Slot =
      createEntryAlloca(getLLVMType(Let.getDeclType()), Let.getName());
  Builder.CreateStore(Init, Slot);
  Scopes.back()[Let.getName()] = Slot;
}

void CodeGenerator::emitIf(const IfStmt &If) {
  llvm::Value *Cond = emitExpr(*If.getCond());

  llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(Ctx, "if.then");
  llvm::BasicBlock *ElseBB =
      If.getElse() ? llvm::BasicBlock::Create(Ctx, "if.else") : nullptr;
  llvm::BasicBlock *EndBB = llvm::BasicBlock::Create(Ctx, "if.end");
  Builder.CreateCondBr(Cond, ThenBB, ElseBB ? ElseBB : EndBB);

  startBlock(ThenBB);
  emitBlock(*If.getThen());
  if (!hasTerminator())
    Builder.CreateBr(EndBB);

  if (ElseBB) {
    startBlock(ElseBB);
    emitStmt(*If.getElse()); // A BlockStmt, or an IfStmt for "else if".
    if (!hasTerminator())
      Builder.CreateBr(EndBB);
  }

  // If both branches returned, nothing jumps to if.end. Drop it rather than
  // leave an empty block that nothing can reach.
  if (llvm::pred_empty(EndBB)) {
    delete EndBB;
    return;
  }
  startBlock(EndBB);
}

void CodeGenerator::emitWhile(const WhileStmt &While) {
  llvm::BasicBlock *CondBB = llvm::BasicBlock::Create(Ctx, "while.cond");
  llvm::BasicBlock *BodyBB = llvm::BasicBlock::Create(Ctx, "while.body");
  llvm::BasicBlock *EndBB = llvm::BasicBlock::Create(Ctx, "while.end");

  Builder.CreateBr(CondBB);

  startBlock(CondBB);
  Builder.CreateCondBr(emitExpr(*While.getCond()), BodyBB, EndBB);

  startBlock(BodyBB);
  emitBlock(*While.getBody());
  if (!hasTerminator())
    Builder.CreateBr(CondBB); // The back edge that makes this a loop.

  startBlock(EndBB);
}

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

llvm::Value *CodeGenerator::emitExpr(const Expr &E) {
  switch (E.getKind()) {
  case Expr::Kind::IntegerLiteral:
    return Builder.getInt32(
        static_cast<uint32_t>(llvm::cast<IntegerLiteralExpr>(E).getValue()));
  case Expr::Kind::BoolLiteral:
    return Builder.getInt1(llvm::cast<BoolLiteralExpr>(E).getValue());
  case Expr::Kind::VarRef: {
    llvm::AllocaInst *Slot = lookupVar(llvm::cast<VarRefExpr>(E).getName());
    return Builder.CreateLoad(Slot->getAllocatedType(), Slot);
  }
  case Expr::Kind::Unary:
    return emitUnary(llvm::cast<UnaryExpr>(E));
  case Expr::Kind::Binary:
    return emitBinary(llvm::cast<BinaryExpr>(E));
  case Expr::Kind::Call:
    return emitCall(llvm::cast<CallExpr>(E));
  }
  llvm_unreachable("unknown expression kind");
}

llvm::Value *CodeGenerator::emitUnary(const UnaryExpr &Unary) {
  llvm::Value *Operand = emitExpr(*Unary.getOperand());
  switch (Unary.getOp()) {
  case UnaryOp::Neg:
    // No nsw flag: negation wraps, so -INT_MIN is INT_MIN, not poison.
    return Builder.CreateNeg(Operand, "neg");
  case UnaryOp::Not:
    return Builder.CreateNot(Operand, "not");
  }
  llvm_unreachable("unknown unary operator");
}

llvm::Value *CodeGenerator::emitBinary(const BinaryExpr &Binary) {
  if (Binary.getOp() == BinaryOp::And || Binary.getOp() == BinaryOp::Or)
    return emitShortCircuit(Binary);

  llvm::Value *LHS = emitExpr(*Binary.getLHS());
  llvm::Value *RHS = emitExpr(*Binary.getRHS());

  // minic integers wrap on overflow (two's complement), so the arithmetic
  // has no nsw/nuw flags. Unlike in C, signed overflow is not undefined.
  switch (Binary.getOp()) {
  case BinaryOp::Add:
    return Builder.CreateAdd(LHS, RHS, "add");
  case BinaryOp::Sub:
    return Builder.CreateSub(LHS, RHS, "sub");
  case BinaryOp::Mul:
    return Builder.CreateMul(LHS, RHS, "mul");
  case BinaryOp::Div:
    return emitDivision(LHS, RHS);
  case BinaryOp::Lt:
    return Builder.CreateICmpSLT(LHS, RHS, "cmp");
  case BinaryOp::Le:
    return Builder.CreateICmpSLE(LHS, RHS, "cmp");
  case BinaryOp::Gt:
    return Builder.CreateICmpSGT(LHS, RHS, "cmp");
  case BinaryOp::Ge:
    return Builder.CreateICmpSGE(LHS, RHS, "cmp");
  case BinaryOp::Eq:
    return Builder.CreateICmpEQ(LHS, RHS, "cmp");
  case BinaryOp::Ne:
    return Builder.CreateICmpNE(LHS, RHS, "cmp");
  case BinaryOp::And:
  case BinaryOp::Or:
    break;
  }
  llvm_unreachable("unknown binary operator");
}

llvm::Value *CodeGenerator::emitShortCircuit(const BinaryExpr &Binary) {
  bool IsAnd = Binary.getOp() == BinaryOp::And;

  llvm::Value *LHS = emitExpr(*Binary.getLHS());
  // Read the current block only after emitting LHS: if LHS itself contains
  // && or ||, it ends in a different block than the one it started in.
  llvm::BasicBlock *LHSEndBB = Builder.GetInsertBlock();

  llvm::BasicBlock *RHSBB =
      llvm::BasicBlock::Create(Ctx, IsAnd ? "and.rhs" : "or.rhs");
  llvm::BasicBlock *EndBB =
      llvm::BasicBlock::Create(Ctx, IsAnd ? "and.end" : "or.end");

  // a && b: if a is false, the answer is false and b is never evaluated.
  // a || b: if a is true, the answer is true and b is never evaluated.
  if (IsAnd)
    Builder.CreateCondBr(LHS, RHSBB, EndBB);
  else
    Builder.CreateCondBr(LHS, EndBB, RHSBB);

  startBlock(RHSBB);
  llvm::Value *RHS = emitExpr(*Binary.getRHS());
  llvm::BasicBlock *RHSEndBB = Builder.GetInsertBlock();
  Builder.CreateBr(EndBB);

  startBlock(EndBB);
  llvm::PHINode *Result =
      Builder.CreatePHI(Builder.getInt1Ty(), 2, IsAnd ? "and" : "or");
  Result->addIncoming(Builder.getInt1(!IsAnd), LHSEndBB);
  Result->addIncoming(RHS, RHSEndBB);
  return Result;
}

llvm::Value *CodeGenerator::emitDivision(llvm::Value *LHS, llvm::Value *RHS) {
  // In LLVM IR, sdiv is undefined behavior for division by zero and for
  // INT_MIN / -1. minic defines both to trap at run time, so the division is
  // guarded unless RHS is a constant that rules both cases out.
  auto *ConstRHS = llvm::dyn_cast<llvm::ConstantInt>(RHS);
  if (!ConstRHS || ConstRHS->isZero() || ConstRHS->isMinusOne()) {
    llvm::Value *IsZero =
        Builder.CreateICmpEQ(RHS, Builder.getInt32(0), "div.iszero");
    llvm::Value *IsMin = Builder.CreateICmpEQ(
        LHS, Builder.getInt32(static_cast<uint32_t>(INT32_MIN)), "div.ismin");
    llvm::Value *IsMinusOne = Builder.CreateICmpEQ(
        RHS, Builder.getInt32(static_cast<uint32_t>(-1)), "div.isminusone");
    llvm::Value *Overflows =
        Builder.CreateAnd(IsMin, IsMinusOne, "div.overflow");
    llvm::Value *Invalid = Builder.CreateOr(IsZero, Overflows, "div.invalid");

    llvm::BasicBlock *OkBB = llvm::BasicBlock::Create(Ctx, "div.ok");
    Builder.CreateCondBr(Invalid, getTrapBlock(), OkBB);
    startBlock(OkBB);
  }
  return Builder.CreateSDiv(LHS, RHS, "div");
}

llvm::Value *CodeGenerator::emitCall(const CallExpr &Call) {
  llvm::Function *Callee = Call.getCallee() == "print"
                               ? getPrintFunction()
                               : M->getFunction(Call.getCallee());

  std::vector<llvm::Value *> Args;
  for (const std::unique_ptr<Expr> &Arg : Call.getArgs())
    Args.push_back(emitExpr(*Arg));

  // A void call produces no value, and LLVM does not allow naming it.
  return Builder.CreateCall(Callee, Args,
                            Callee->getReturnType()->isVoidTy() ? "" : "call");
}

//===----------------------------------------------------------------------===//
// Helpers
//===----------------------------------------------------------------------===//

llvm::AllocaInst *CodeGenerator::createEntryAlloca(llvm::Type *Ty,
                                                   const llvm::Twine &Name) {
  // mem2reg only promotes allocas in the entry block, so every alloca goes
  // there, after the ones already created, wherever the variable is declared.
  llvm::BasicBlock &Entry = CurFn->getEntryBlock();
  auto InsertPt = Entry.begin();
  while (InsertPt != Entry.end() && llvm::isa<llvm::AllocaInst>(*InsertPt))
    ++InsertPt;
  llvm::IRBuilder<> EntryBuilder(&Entry, InsertPt);
  return EntryBuilder.CreateAlloca(Ty, nullptr, Name);
}

llvm::AllocaInst *CodeGenerator::lookupVar(llvm::StringRef Name) const {
  for (auto ScopeIt = Scopes.rbegin(); ScopeIt != Scopes.rend(); ++ScopeIt) {
    auto It = ScopeIt->find(Name);
    if (It != ScopeIt->end())
      return It->second;
  }
  llvm_unreachable("Sema guarantees every variable is declared");
}

void CodeGenerator::startBlock(llvm::BasicBlock *BB) {
  CurFn->insert(CurFn->end(), BB);
  Builder.SetInsertPoint(BB);
}

bool CodeGenerator::hasTerminator() const {
  return Builder.GetInsertBlock()->getTerminator() != nullptr;
}

llvm::Function *CodeGenerator::getPrintFunction() {
  // print(int) is implemented by minic_print in the runtime library, which
  // is linked into every program.
  llvm::FunctionCallee Callee = M->getOrInsertFunction(
      "minic_print", Builder.getVoidTy(), Builder.getInt32Ty());
  return llvm::cast<llvm::Function>(Callee.getCallee());
}

llvm::BasicBlock *CodeGenerator::getTrapBlock() {
  if (!TrapBB) {
    TrapBB = llvm::BasicBlock::Create(Ctx, "trap");
    llvm::IRBuilder<> TrapBuilder(TrapBB);
    TrapBuilder.CreateCall(
        llvm::Intrinsic::getDeclaration(M.get(), llvm::Intrinsic::trap));
    TrapBuilder.CreateUnreachable();
  }
  return TrapBB;
}

std::unique_ptr<llvm::Module> minic::generateIR(const Program &Prog,
                                                llvm::LLVMContext &Ctx,
                                                llvm::StringRef ModuleName) {
  return CodeGenerator(Ctx, ModuleName).run(Prog);
}