//===- Parser.h - minic recursive-descent parser ------------------*- C++
//-*-===//
//
// Builds an AST from the token stream. Each grammar rule in docs/grammar.md
// has one parse method; binary expressions use precedence climbing.
//
// On a syntax error the parser reports it and recovers (panic mode), so one
// run reports as many independent errors as possible.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_PARSER_H
#define MINIC_PARSER_H

#include "minic/AST.h"
#include "minic/Token.h"

#include <memory>
#include <optional>

namespace minic {

class DiagnosticEngine;
class Lexer;

class Parser {
public:
  Parser(Lexer &Lex, DiagnosticEngine &Diags);

  /// Parses a whole file. Always returns a Program; it contains every
  /// function that could be parsed, even if errors were reported.
  std::unique_ptr<Program> parseProgram();

private:
  // Token handling.
  Token lexSkippingUnknown();
  void consume();
  bool consumeIf(tok::TokenKind Kind);
  bool expect(tok::TokenKind Kind);
  std::optional<Token> expectIdentifier();

  // Error recovery.
  void skipToStatementEnd();
  void skipToNextFunction();

  // Declarations.
  std::unique_ptr<FunctionDecl> parseFunction();
  std::optional<ParamDecl> parseParam();
  std::optional<Type> parseType();

  // Statements.
  std::unique_ptr<BlockStmt> parseBlock();
  std::unique_ptr<Stmt> parseStatement();
  std::unique_ptr<Stmt> parseLetStmt();
  std::unique_ptr<Stmt> parseAssignStmt();
  std::unique_ptr<Stmt> parseIfStmt();
  std::unique_ptr<Stmt> parseWhileStmt();
  std::unique_ptr<Stmt> parseReturnStmt();
  std::unique_ptr<Stmt> parseExprStmt();

  // Expressions.
  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<Expr> parseBinaryExpr(int MinPrecedence);
  std::unique_ptr<Expr> parseUnaryExpr();
  std::unique_ptr<Expr> parsePrimaryExpr();
  std::unique_ptr<Expr> parseCallExpr();

  Lexer &Lex;
  DiagnosticEngine &Diags;

  /// The current token and the one after it (two-token lookahead).
  Token Tok;
  Token NextTok;

  /// End of the last consumed token, used to place "expected ';'" errors.
  const char *PrevTokenEnd = nullptr;
};

} // namespace minic

#endif // MINIC_PARSER_H