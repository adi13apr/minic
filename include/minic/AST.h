//===- AST.h - minic abstract syntax tree -------------------------*- C++
//-*-===//
//
// The tree the parser builds. Every node owns its children through
// std::unique_ptr. Names and literal spellings are StringRefs into the source
// buffer owned by llvm::SourceMgr, so the AST must not outlive that SourceMgr.
//
// Nodes support LLVM-style RTTI: use llvm::isa<>, llvm::cast<>, and
// llvm::dyn_cast<> instead of dynamic_cast.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_AST_H
#define MINIC_AST_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/SMLoc.h"

#include <memory>
#include <utility>
#include <vector>

namespace minic {

enum class Type { Unknown, Void, Int, Bool };
const char *getTypeName(Type T);

enum class UnaryOp { Neg, Not };
enum class BinaryOp { Mul, Div, Add, Sub, Lt, Le, Gt, Ge, Eq, Ne, And, Or };
const char *getUnaryOpSpelling(UnaryOp Op);
const char *getBinaryOpSpelling(BinaryOp Op);

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

class Expr {
public:
  enum class Kind { IntegerLiteral, BoolLiteral, VarRef, Unary, Binary, Call };

  Expr(const Expr &) = delete;
  Expr &operator=(const Expr &) = delete;
  virtual ~Expr() = default;

  Kind getKind() const { return TheKind; }
  llvm::SMLoc getLoc() const { return Loc; }

  /// The type computed by Sema; Type::Unknown until Sema runs.
  Type getType() const { return Ty; }
  void setType(Type T) { Ty = T; }

protected:
  Expr(Kind K, llvm::SMLoc Loc) : TheKind(K), Loc(Loc) {}

private:
  const Kind TheKind;
  llvm::SMLoc Loc;
  Type Ty = Type::Unknown;
};

class IntegerLiteralExpr final : public Expr {
public:
  IntegerLiteralExpr(llvm::StringRef Spelling, llvm::SMLoc Loc)
      : Expr(Kind::IntegerLiteral, Loc), Spelling(Spelling) {}

  /// The digits as written; Sema converts and range-checks them.
  llvm::StringRef getSpelling() const { return Spelling; }

  static bool classof(const Expr *E) {
    return E->getKind() == Kind::IntegerLiteral;
  }

private:
  llvm::StringRef Spelling;
};

class BoolLiteralExpr final : public Expr {
public:
  BoolLiteralExpr(bool Value, llvm::SMLoc Loc)
      : Expr(Kind::BoolLiteral, Loc), Value(Value) {}

  bool getValue() const { return Value; }

  static bool classof(const Expr *E) {
    return E->getKind() == Kind::BoolLiteral;
  }

private:
  bool Value;
};

class VarRefExpr final : public Expr {
public:
  VarRefExpr(llvm::StringRef Name, llvm::SMLoc Loc)
      : Expr(Kind::VarRef, Loc), Name(Name) {}

  llvm::StringRef getName() const { return Name; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::VarRef; }

private:
  llvm::StringRef Name;
};

class UnaryExpr final : public Expr {
public:
  UnaryExpr(UnaryOp Op, std::unique_ptr<Expr> Operand, llvm::SMLoc Loc)
      : Expr(Kind::Unary, Loc), Op(Op), Operand(std::move(Operand)) {}

  UnaryOp getOp() const { return Op; }
  Expr *getOperand() const { return Operand.get(); }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Unary; }

private:
  UnaryOp Op;
  std::unique_ptr<Expr> Operand;
};

class BinaryExpr final : public Expr {
public:
  BinaryExpr(BinaryOp Op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS,
             llvm::SMLoc Loc)
      : Expr(Kind::Binary, Loc), Op(Op), LHS(std::move(LHS)),
        RHS(std::move(RHS)) {}

  BinaryOp getOp() const { return Op; }
  Expr *getLHS() const { return LHS.get(); }
  Expr *getRHS() const { return RHS.get(); }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Binary; }

private:
  BinaryOp Op;
  std::unique_ptr<Expr> LHS;
  std::unique_ptr<Expr> RHS;
};

class CallExpr final : public Expr {
public:
  CallExpr(llvm::StringRef Callee, std::vector<std::unique_ptr<Expr>> Args,
           llvm::SMLoc Loc)
      : Expr(Kind::Call, Loc), Callee(Callee), Args(std::move(Args)) {}

  llvm::StringRef getCallee() const { return Callee; }
  llvm::ArrayRef<std::unique_ptr<Expr>> getArgs() const { return Args; }

  static bool classof(const Expr *E) { return E->getKind() == Kind::Call; }

private:
  llvm::StringRef Callee;
  std::vector<std::unique_ptr<Expr>> Args;
};

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

class Stmt {
public:
  enum class Kind { Block, Let, Assign, If, While, Return, ExprStmt };

  Stmt(const Stmt &) = delete;
  Stmt &operator=(const Stmt &) = delete;
  virtual ~Stmt() = default;

  Kind getKind() const { return TheKind; }
  llvm::SMLoc getLoc() const { return Loc; }

protected:
  Stmt(Kind K, llvm::SMLoc Loc) : TheKind(K), Loc(Loc) {}

private:
  const Kind TheKind;
  llvm::SMLoc Loc;
};

class BlockStmt final : public Stmt {
public:
  BlockStmt(std::vector<std::unique_ptr<Stmt>> Body, llvm::SMLoc Loc)
      : Stmt(Kind::Block, Loc), Body(std::move(Body)) {}

  llvm::ArrayRef<std::unique_ptr<Stmt>> getBody() const { return Body; }

  static bool classof(const Stmt *S) { return S->getKind() == Kind::Block; }

private:
  std::vector<std::unique_ptr<Stmt>> Body;
};

class LetStmt final : public Stmt {
public:
  LetStmt(llvm::StringRef Name, llvm::SMLoc NameLoc, Type DeclType,
          std::unique_ptr<Expr> Init, llvm::SMLoc Loc)
      : Stmt(Kind::Let, Loc), Name(Name), NameLoc(NameLoc), DeclType(DeclType),
        Init(std::move(Init)) {}

  llvm::StringRef getName() const { return Name; }
  llvm::SMLoc getNameLoc() const { return NameLoc; }
  Type getDeclType() const { return DeclType; }
  Expr *getInit() const { return Init.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == Kind::Let; }

private:
  llvm::StringRef Name;
  llvm::SMLoc NameLoc;
  Type DeclType;
  std::unique_ptr<Expr> Init;
};

class AssignStmt final : public Stmt {
public:
  AssignStmt(llvm::StringRef Name, std::unique_ptr<Expr> Value, llvm::SMLoc Loc)
      : Stmt(Kind::Assign, Loc), Name(Name), Value(std::move(Value)) {}

  llvm::StringRef getName() const { return Name; }
  Expr *getValue() const { return Value.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == Kind::Assign; }

private:
  llvm::StringRef Name;
  std::unique_ptr<Expr> Value;
};

class IfStmt final : public Stmt {
public:
  IfStmt(std::unique_ptr<Expr> Cond, std::unique_ptr<BlockStmt> Then,
         std::unique_ptr<Stmt> Else, llvm::SMLoc Loc)
      : Stmt(Kind::If, Loc), Cond(std::move(Cond)), Then(std::move(Then)),
        Else(std::move(Else)) {}

  Expr *getCond() const { return Cond.get(); }
  BlockStmt *getThen() const { return Then.get(); }
  /// The else branch: null, a BlockStmt, or another IfStmt (for "else if").
  Stmt *getElse() const { return Else.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == Kind::If; }

private:
  std::unique_ptr<Expr> Cond;
  std::unique_ptr<BlockStmt> Then;
  std::unique_ptr<Stmt> Else;
};

class WhileStmt final : public Stmt {
public:
  WhileStmt(std::unique_ptr<Expr> Cond, std::unique_ptr<BlockStmt> Body,
            llvm::SMLoc Loc)
      : Stmt(Kind::While, Loc), Cond(std::move(Cond)), Body(std::move(Body)) {}

  Expr *getCond() const { return Cond.get(); }
  BlockStmt *getBody() const { return Body.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == Kind::While; }

private:
  std::unique_ptr<Expr> Cond;
  std::unique_ptr<BlockStmt> Body;
};

class ReturnStmt final : public Stmt {
public:
  ReturnStmt(std::unique_ptr<Expr> Value, llvm::SMLoc Loc)
      : Stmt(Kind::Return, Loc), Value(std::move(Value)) {}

  /// The returned value, or null for "return;".
  Expr *getValue() const { return Value.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == Kind::Return; }

private:
  std::unique_ptr<Expr> Value;
};

class ExprStmt final : public Stmt {
public:
  ExprStmt(std::unique_ptr<Expr> E, llvm::SMLoc Loc)
      : Stmt(Kind::ExprStmt, Loc), E(std::move(E)) {}

  Expr *getExpr() const { return E.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == Kind::ExprStmt; }

private:
  std::unique_ptr<Expr> E;
};

//===----------------------------------------------------------------------===//
// Declarations
//===----------------------------------------------------------------------===//

struct ParamDecl {
  llvm::StringRef Name;
  llvm::SMLoc Loc;
  Type Ty;
};

class FunctionDecl {
public:
  FunctionDecl(llvm::StringRef Name, llvm::SMLoc Loc,
               std::vector<ParamDecl> Params, Type ReturnType,
               std::unique_ptr<BlockStmt> Body)
      : Name(Name), Loc(Loc), Params(std::move(Params)), ReturnType(ReturnType),
        Body(std::move(Body)) {}

  llvm::StringRef getName() const { return Name; }
  llvm::SMLoc getLoc() const { return Loc; }
  llvm::ArrayRef<ParamDecl> getParams() const { return Params; }
  Type getReturnType() const { return ReturnType; }
  BlockStmt *getBody() const { return Body.get(); }

private:
  llvm::StringRef Name;
  llvm::SMLoc Loc;
  std::vector<ParamDecl> Params;
  Type ReturnType;
  std::unique_ptr<BlockStmt> Body;
};

class Program {
public:
  void addFunction(std::unique_ptr<FunctionDecl> Fn) {
    Functions.push_back(std::move(Fn));
  }
  llvm::ArrayRef<std::unique_ptr<FunctionDecl>> getFunctions() const {
    return Functions;
  }

private:
  std::vector<std::unique_ptr<FunctionDecl>> Functions;
};

} // namespace minic

#endif // MINIC_AST_H