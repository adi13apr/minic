//===- ASTDumper.cpp - Print a minic AST ----------------------------------===//

#include "minic/ASTDumper.h"
#include "minic/AST.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace minic;

namespace {

class ASTDumper {
public:
  explicit ASTDumper(llvm::raw_ostream &OS) : OS(OS) {}

  void dump(const Program &Prog);

private:
  void dumpFunction(const FunctionDecl &Fn);
  void dumpStmt(const Stmt &S, unsigned Depth);
  void dumpExpr(const Expr &E);
  void startLine(unsigned Depth) { OS.indent(Depth * 2); }

  llvm::raw_ostream &OS;
};

} // namespace

void ASTDumper::dump(const Program &Prog) {
  for (const std::unique_ptr<FunctionDecl> &Fn : Prog.getFunctions())
    dumpFunction(*Fn);
}

void ASTDumper::dumpFunction(const FunctionDecl &Fn) {
  OS << "FunctionDecl " << Fn.getName() << '(';
  llvm::interleaveComma(Fn.getParams(), OS, [&](const ParamDecl &Param) {
    OS << Param.Name << ": " << getTypeName(Param.Ty);
  });
  OS << ") -> " << getTypeName(Fn.getReturnType()) << '\n';
  dumpStmt(*Fn.getBody(), 1);
}

void ASTDumper::dumpStmt(const Stmt &S, unsigned Depth) {
  startLine(Depth);
  switch (S.getKind()) {
  case Stmt::Kind::Block:
    OS << "Block\n";
    for (const std::unique_ptr<Stmt> &Child :
         llvm::cast<BlockStmt>(S).getBody())
      dumpStmt(*Child, Depth + 1);
    return;
  case Stmt::Kind::Let: {
    const auto &Let = llvm::cast<LetStmt>(S);
    OS << "Let " << Let.getName() << ": " << getTypeName(Let.getDeclType())
       << " = ";
    dumpExpr(*Let.getInit());
    OS << '\n';
    return;
  }
  case Stmt::Kind::Assign: {
    const auto &Assign = llvm::cast<AssignStmt>(S);
    OS << "Assign " << Assign.getName() << " = ";
    dumpExpr(*Assign.getValue());
    OS << '\n';
    return;
  }
  case Stmt::Kind::If: {
    const auto &If = llvm::cast<IfStmt>(S);
    OS << "If ";
    dumpExpr(*If.getCond());
    OS << '\n';
    dumpStmt(*If.getThen(), Depth + 1);
    if (const Stmt *Else = If.getElse()) {
      startLine(Depth + 1);
      OS << "Else\n";
      dumpStmt(*Else, Depth + 2);
    }
    return;
  }
  case Stmt::Kind::While: {
    const auto &While = llvm::cast<WhileStmt>(S);
    OS << "While ";
    dumpExpr(*While.getCond());
    OS << '\n';
    dumpStmt(*While.getBody(), Depth + 1);
    return;
  }
  case Stmt::Kind::Return: {
    OS << "Return";
    if (const Expr *Value = llvm::cast<ReturnStmt>(S).getValue()) {
      OS << ' ';
      dumpExpr(*Value);
    }
    OS << '\n';
    return;
  }
  case Stmt::Kind::ExprStmt:
    OS << "ExprStmt ";
    dumpExpr(*llvm::cast<ExprStmt>(S).getExpr());
    OS << '\n';
    return;
  }
  llvm_unreachable("unknown statement kind");
}

void ASTDumper::dumpExpr(const Expr &E) {
  switch (E.getKind()) {
  case Expr::Kind::IntegerLiteral:
    OS << llvm::cast<IntegerLiteralExpr>(E).getSpelling();
    return;
  case Expr::Kind::BoolLiteral:
    OS << (llvm::cast<BoolLiteralExpr>(E).getValue() ? "true" : "false");
    return;
  case Expr::Kind::VarRef:
    OS << llvm::cast<VarRefExpr>(E).getName();
    return;
  case Expr::Kind::Unary: {
    const auto &Unary = llvm::cast<UnaryExpr>(E);
    OS << '(' << getUnaryOpSpelling(Unary.getOp()) << ' ';
    dumpExpr(*Unary.getOperand());
    OS << ')';
    return;
  }
  case Expr::Kind::Binary: {
    const auto &Binary = llvm::cast<BinaryExpr>(E);
    OS << '(' << getBinaryOpSpelling(Binary.getOp()) << ' ';
    dumpExpr(*Binary.getLHS());
    OS << ' ';
    dumpExpr(*Binary.getRHS());
    OS << ')';
    return;
  }
  case Expr::Kind::Call: {
    const auto &Call = llvm::cast<CallExpr>(E);
    OS << "(call " << Call.getCallee();
    for (const std::unique_ptr<Expr> &Arg : Call.getArgs()) {
      OS << ' ';
      dumpExpr(*Arg);
    }
    OS << ')';
    return;
  }
  }
  llvm_unreachable("unknown expression kind");
}

void minic::dumpAST(const Program &Prog, llvm::raw_ostream &OS) {
  ASTDumper(OS).dump(Prog);
}