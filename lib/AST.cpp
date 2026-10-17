//===- AST.cpp - minic abstract syntax tree -------------------------------===//

#include "minic/AST.h"

#include "llvm/Support/ErrorHandling.h"

using namespace minic;

const char *minic::getTypeName(Type T) {
  switch (T) {
  case Type::Unknown:
    return "<unknown>";
  case Type::Void:
    return "void";
  case Type::Int:
    return "int";
  case Type::Bool:
    return "bool";
  }
  llvm_unreachable("unknown type");
}

const char *minic::getUnaryOpSpelling(UnaryOp Op) {
  switch (Op) {
  case UnaryOp::Neg:
    return "-";
  case UnaryOp::Not:
    return "!";
  }
  llvm_unreachable("unknown unary operator");
}

const char *minic::getBinaryOpSpelling(BinaryOp Op) {
  switch (Op) {
  case BinaryOp::Mul:
    return "*";
  case BinaryOp::Div:
    return "/";
  case BinaryOp::Add:
    return "+";
  case BinaryOp::Sub:
    return "-";
  case BinaryOp::Lt:
    return "<";
  case BinaryOp::Le:
    return "<=";
  case BinaryOp::Gt:
    return ">";
  case BinaryOp::Ge:
    return ">=";
  case BinaryOp::Eq:
    return "==";
  case BinaryOp::Ne:
    return "!=";
  case BinaryOp::And:
    return "&&";
  case BinaryOp::Or:
    return "||";
  }
  llvm_unreachable("unknown binary operator");
}