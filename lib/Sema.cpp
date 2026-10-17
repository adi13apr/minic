//===- Sema.cpp - minic semantic analysis ---------------------------------===//

#include "minic/Sema.h"
#include "minic/AST.h"
#include "minic/Diagnostic.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/ErrorHandling.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace minic;

namespace {

struct VarInfo {
  Type Ty;
  llvm::SMLoc DeclLoc;
};

using Scope = llvm::StringMap<VarInfo>;

/// Pushes a scope on construction and pops it on destruction, so the scope
/// stack stays balanced on every path out of a block.
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

class SemaChecker {
public:
  SemaChecker(DiagnosticEngine &Diags, const SemaOptions &Opts)
      : Diags(Diags), Opts(Opts) {}

  void checkProgram(Program &Prog);

private:
  void collectFunctions(const Program &Prog);
  void checkMain();
  void checkFunction(const FunctionDecl &Fn);

  void checkBlock(const BlockStmt &Block, bool NewScope);
  void checkStmt(Stmt &S);
  void checkLet(LetStmt &Let);
  void checkAssign(AssignStmt &Assign);
  void checkReturn(ReturnStmt &Ret);
  void checkCondition(Expr &Cond);

  Type checkExpr(Expr &E);
  Type checkIntegerLiteral(IntegerLiteralExpr &Lit, bool Negated);
  Type checkUnary(UnaryExpr &Unary);
  Type checkBinary(BinaryExpr &Binary);
  Type checkCall(CallExpr &Call);

  void declareVar(llvm::StringRef Name, Type Ty, llvm::SMLoc Loc);
  const VarInfo *lookupVar(llvm::StringRef Name) const;

  DiagnosticEngine &Diags;
  const SemaOptions &Opts;
  llvm::StringMap<const FunctionDecl *> Functions;
  std::vector<Scope> Scopes;
  const FunctionDecl *CurFn = nullptr;
};

} // namespace

/// Conservative check that a statement returns on every path through it.
/// A loop never counts, even "while (true)": deciding whether a loop
/// terminates is impossible in general, so the check errs on the safe side.
static bool alwaysReturns(const Stmt &S) {
  switch (S.getKind()) {
  case Stmt::Kind::Return:
    return true;
  case Stmt::Kind::Block:
    return llvm::any_of(llvm::cast<BlockStmt>(S).getBody(),
                        [](const std::unique_ptr<Stmt> &Child) {
                          return alwaysReturns(*Child);
                        });
  case Stmt::Kind::If: {
    const auto &If = llvm::cast<IfStmt>(S);
    return If.getElse() && alwaysReturns(*If.getThen()) &&
           alwaysReturns(*If.getElse());
  }
  default:
    return false;
  }
}

//===----------------------------------------------------------------------===//
// Declarations
//===----------------------------------------------------------------------===//

void SemaChecker::checkProgram(Program &Prog) {
  // Pass 1: record every function signature, so a call may refer to a
  // function defined later in the file, or to the function itself.
  collectFunctions(Prog);
  if (Opts.RequireMain)
    checkMain();

  // Pass 2: check every function body.
  for (const std::unique_ptr<FunctionDecl> &Fn : Prog.getFunctions())
    checkFunction(*Fn);
}

void SemaChecker::collectFunctions(const Program &Prog) {
  for (const std::unique_ptr<FunctionDecl> &Fn : Prog.getFunctions()) {
    if (Fn->getName() == "print") {
      Diags.error(Fn->getLoc(),
                  "'print' is a builtin function and cannot be redefined");
      continue;
    }
    auto [It, Inserted] = Functions.try_emplace(Fn->getName(), Fn.get());
    if (!Inserted) {
      Diags.error(Fn->getLoc(),
                  "redefinition of function '" + Fn->getName() + "'");
      Diags.note(It->second->getLoc(), "previous definition is here");
    }
  }
}

void SemaChecker::checkMain() {
  auto It = Functions.find("main");
  if (It == Functions.end()) {
    Diags.error(llvm::SMLoc(), "program has no 'main' function");
    return;
  }
  const FunctionDecl &Main = *It->second;
  if (!Main.getParams().empty() || Main.getReturnType() != Type::Int)
    Diags.error(Main.getLoc(), "'main' must be declared as 'fn main() -> int'");
}

void SemaChecker::checkFunction(const FunctionDecl &Fn) {
  CurFn = &Fn;

  // Parameters and the function's outermost block share one scope, so the
  // body cannot redeclare a parameter at its top level.
  ScopeGuard Guard(Scopes);
  for (const ParamDecl &Param : Fn.getParams())
    declareVar(Param.Name, Param.Ty, Param.Loc);
  checkBlock(*Fn.getBody(), /*NewScope=*/false);

  if (Fn.getReturnType() != Type::Void && !alwaysReturns(*Fn.getBody()))
    Diags.error(Fn.getLoc(), "non-void function '" + Fn.getName() +
                                 "' does not return a value on every path");

  CurFn = nullptr;
}

//===----------------------------------------------------------------------===//
// Scopes
//===----------------------------------------------------------------------===//

void SemaChecker::declareVar(llvm::StringRef Name, Type Ty, llvm::SMLoc Loc) {
  auto [It, Inserted] = Scopes.back().try_emplace(Name, VarInfo{Ty, Loc});
  if (!Inserted) {
    Diags.error(Loc, "redeclaration of '" + Name + "'");
    Diags.note(It->second.DeclLoc, "previous declaration is here");
  }
}

const VarInfo *SemaChecker::lookupVar(llvm::StringRef Name) const {
  // Search from the innermost scope outwards, so inner names shadow outer.
  for (auto ScopeIt = Scopes.rbegin(); ScopeIt != Scopes.rend(); ++ScopeIt) {
    auto It = ScopeIt->find(Name);
    if (It != ScopeIt->end())
      return &It->second;
  }
  return nullptr;
}

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

void SemaChecker::checkBlock(const BlockStmt &Block, bool NewScope) {
  std::optional<ScopeGuard> Guard;
  if (NewScope)
    Guard.emplace(Scopes);
  for (const std::unique_ptr<Stmt> &S : Block.getBody())
    checkStmt(*S);
}

void SemaChecker::checkStmt(Stmt &S) {
  switch (S.getKind()) {
  case Stmt::Kind::Block:
    checkBlock(llvm::cast<BlockStmt>(S), /*NewScope=*/true);
    return;
  case Stmt::Kind::Let:
    checkLet(llvm::cast<LetStmt>(S));
    return;
  case Stmt::Kind::Assign:
    checkAssign(llvm::cast<AssignStmt>(S));
    return;
  case Stmt::Kind::If: {
    auto &If = llvm::cast<IfStmt>(S);
    checkCondition(*If.getCond());
    checkBlock(*If.getThen(), /*NewScope=*/true);
    if (Stmt *Else = If.getElse())
      checkStmt(*Else);
    return;
  }
  case Stmt::Kind::While: {
    auto &While = llvm::cast<WhileStmt>(S);
    checkCondition(*While.getCond());
    checkBlock(*While.getBody(), /*NewScope=*/true);
    return;
  }
  case Stmt::Kind::Return:
    checkReturn(llvm::cast<ReturnStmt>(S));
    return;
  case Stmt::Kind::ExprStmt: {
    Expr *E = llvm::cast<ExprStmt>(S).getExpr();
    checkExpr(*E);
    if (!llvm::isa<CallExpr>(E))
      Diags.error(S.getLoc(), "expression result is unused; only function "
                              "calls can be used as statements");
    return;
  }
  }
  llvm_unreachable("unknown statement kind");
}

void SemaChecker::checkLet(LetStmt &Let) {
  // Check the initializer before declaring the name, so "let x: int = x;"
  // cannot refer to the variable that is being declared.
  Type InitTy = checkExpr(*Let.getInit());
  if (InitTy != Type::Error && InitTy != Let.getDeclType())
    Diags.error(Let.getInit()->getLoc(),
                llvm::Twine("cannot initialize variable '") + Let.getName() +
                    "' of type '" + getTypeName(Let.getDeclType()) +
                    "' with a value of type '" + getTypeName(InitTy) + "'");
  declareVar(Let.getName(), Let.getDeclType(), Let.getNameLoc());
}

void SemaChecker::checkAssign(AssignStmt &Assign) {
  Type ValueTy = checkExpr(*Assign.getValue());
  const VarInfo *Var = lookupVar(Assign.getName());
  if (!Var) {
    Diags.error(Assign.getLoc(),
                "use of undeclared variable '" + Assign.getName() + "'");
    return;
  }
  if (ValueTy != Type::Error && ValueTy != Var->Ty)
    Diags.error(Assign.getValue()->getLoc(),
                llvm::Twine("cannot assign a value of type '") +
                    getTypeName(ValueTy) + "' to variable '" +
                    Assign.getName() + "' of type '" + getTypeName(Var->Ty) +
                    "'");
}

void SemaChecker::checkReturn(ReturnStmt &Ret) {
  Type ReturnTy = CurFn->getReturnType();
  Expr *Value = Ret.getValue();

  if (!Value) {
    if (ReturnTy != Type::Void)
      Diags.error(Ret.getLoc(), "non-void function '" + CurFn->getName() +
                                    "' must return a value");
    return;
  }

  Type ValueTy = checkExpr(*Value);
  if (ReturnTy == Type::Void) {
    Diags.error(Value->getLoc(), "void function '" + CurFn->getName() +
                                     "' cannot return a value");
    return;
  }
  if (ValueTy != Type::Error && ValueTy != ReturnTy)
    Diags.error(Value->getLoc(),
                llvm::Twine("cannot return a value of type '") +
                    getTypeName(ValueTy) + "' from function '" +
                    CurFn->getName() + "' with return type '" +
                    getTypeName(ReturnTy) + "'");
}

void SemaChecker::checkCondition(Expr &Cond) {
  Type CondTy = checkExpr(Cond);
  if (CondTy != Type::Error && CondTy != Type::Bool)
    Diags.error(Cond.getLoc(),
                llvm::Twine("condition must be of type 'bool', found '") +
                    getTypeName(CondTy) + "'");
}

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

Type SemaChecker::checkExpr(Expr &E) {
  Type Ty = Type::Error;
  switch (E.getKind()) {
  case Expr::Kind::IntegerLiteral:
    Ty = checkIntegerLiteral(llvm::cast<IntegerLiteralExpr>(E),
                             /*Negated=*/false);
    break;
  case Expr::Kind::BoolLiteral:
    Ty = Type::Bool;
    break;
  case Expr::Kind::VarRef: {
    auto &Ref = llvm::cast<VarRefExpr>(E);
    if (const VarInfo *Var = lookupVar(Ref.getName()))
      Ty = Var->Ty;
    else
      Diags.error(Ref.getLoc(),
                  "use of undeclared variable '" + Ref.getName() + "'");
    break;
  }
  case Expr::Kind::Unary:
    Ty = checkUnary(llvm::cast<UnaryExpr>(E));
    break;
  case Expr::Kind::Binary:
    Ty = checkBinary(llvm::cast<BinaryExpr>(E));
    break;
  case Expr::Kind::Call:
    Ty = checkCall(llvm::cast<CallExpr>(E));
    break;
  }
  E.setType(Ty);
  return Ty;
}

Type SemaChecker::checkIntegerLiteral(IntegerLiteralExpr &Lit, bool Negated) {
  // -2147483648 is unary minus applied to 2147483648, which alone does not
  // fit in an int. A literal directly under unary minus may therefore be one
  // larger than INT32_MAX.
  const uint64_t Limit =
      Negated ? uint64_t(INT32_MAX) + 1 : uint64_t(INT32_MAX);

  uint64_t Value = 0;
  // getAsInteger returns true on failure, including overflow of uint64_t.
  if (Lit.getSpelling().getAsInteger(10, Value) || Value > Limit) {
    Diags.error(Lit.getLoc(), "integer literal '" + Lit.getSpelling() +
                                  "' is too large for type 'int'");
    Value = 0;
  }
  Lit.setValue(Value);
  Lit.setType(Type::Int);
  return Type::Int;
}

Type SemaChecker::checkUnary(UnaryExpr &Unary) {
  Expr &Operand = *Unary.getOperand();
  Type OperandTy;
  if (Unary.getOp() == UnaryOp::Neg && llvm::isa<IntegerLiteralExpr>(Operand))
    OperandTy = checkIntegerLiteral(llvm::cast<IntegerLiteralExpr>(Operand),
                                    /*Negated=*/true);
  else
    OperandTy = checkExpr(Operand);

  Type Expected = Unary.getOp() == UnaryOp::Neg ? Type::Int : Type::Bool;
  if (OperandTy != Type::Error && OperandTy != Expected)
    Diags.error(Unary.getLoc(), llvm::Twine("invalid operand to unary '") +
                                    getUnaryOpSpelling(Unary.getOp()) +
                                    "': expected '" + getTypeName(Expected) +
                                    "', found '" + getTypeName(OperandTy) +
                                    "'");
  return Expected;
}

Type SemaChecker::checkBinary(BinaryExpr &Binary) {
  Type LHSTy = checkExpr(*Binary.getLHS());
  Type RHSTy = checkExpr(*Binary.getRHS());

  Type OperandTy = Type::Int;
  Type ResultTy = Type::Bool;
  bool IsEquality = false;
  switch (Binary.getOp()) {
  case BinaryOp::Mul:
  case BinaryOp::Div:
  case BinaryOp::Add:
  case BinaryOp::Sub:
    ResultTy = Type::Int;
    break;
  case BinaryOp::Lt:
  case BinaryOp::Le:
  case BinaryOp::Gt:
  case BinaryOp::Ge:
    break;
  case BinaryOp::Eq:
  case BinaryOp::Ne:
    IsEquality = true;
    break;
  case BinaryOp::And:
  case BinaryOp::Or:
    OperandTy = Type::Bool;
    break;
  }

  // The result type depends only on the operator, so even an invalid
  // expression has a known type and causes no follow-on errors.
  if (LHSTy == Type::Error || RHSTy == Type::Error)
    return ResultTy;

  bool Valid =
      IsEquality ? LHSTy == RHSTy && (LHSTy == Type::Int || LHSTy == Type::Bool)
                 : LHSTy == OperandTy && RHSTy == OperandTy;
  if (!Valid)
    Diags.error(Binary.getLoc(), llvm::Twine("invalid operands to binary '") +
                                     getBinaryOpSpelling(Binary.getOp()) +
                                     "' ('" + getTypeName(LHSTy) + "' and '" +
                                     getTypeName(RHSTy) + "')");
  return ResultTy;
}

Type SemaChecker::checkCall(CallExpr &Call) {
  llvm::SmallVector<Type, 4> ArgTypes;
  for (const std::unique_ptr<Expr> &Arg : Call.getArgs())
    ArgTypes.push_back(checkExpr(*Arg));

  // print(int) is the only builtin; it has no FunctionDecl of its own.
  static const ParamDecl PrintParams[] = {{"value", llvm::SMLoc(), Type::Int}};

  llvm::ArrayRef<ParamDecl> Params;
  Type ReturnTy;
  if (Call.getCallee() == "print") {
    if (!Opts.AllowPrint) {
      Diags.error(Call.getLoc(),
                  "'print' is not available when compiling for the GPU");
      return Type::Void;
    }
    Params = PrintParams;
    ReturnTy = Type::Void;
  } else {
    auto It = Functions.find(Call.getCallee());
    if (It == Functions.end()) {
      Diags.error(Call.getLoc(),
                  "call to undeclared function '" + Call.getCallee() + "'");
      return Type::Error;
    }
    Params = It->second->getParams();
    ReturnTy = It->second->getReturnType();
  }

  if (ArgTypes.size() != Params.size()) {
    Diags.error(Call.getLoc(),
                llvm::Twine("function '") + Call.getCallee() + "' expects " +
                    std::to_string(Params.size()) +
                    (Params.size() == 1 ? " argument" : " arguments") +
                    ", but got " + std::to_string(ArgTypes.size()));
    return ReturnTy;
  }

  for (std::size_t I = 0; I < Params.size(); ++I) {
    if (ArgTypes[I] == Type::Error || ArgTypes[I] == Params[I].Ty)
      continue;
    Diags.error(Call.getArgs()[I]->getLoc(),
                llvm::Twine("argument ") + std::to_string(I + 1) +
                    " of call to '" + Call.getCallee() + "' has type '" +
                    getTypeName(ArgTypes[I]) + "', but parameter '" +
                    Params[I].Name + "' has type '" +
                    getTypeName(Params[I].Ty) + "'");
  }
  return ReturnTy;
}

void minic::checkProgram(Program &Prog, DiagnosticEngine &Diags,
                         const SemaOptions &Opts) {
  SemaChecker(Diags, Opts).checkProgram(Prog);
}
