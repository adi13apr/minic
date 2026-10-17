//===- Parser.cpp - minic recursive-descent parser ------------------------===//

#include "minic/Parser.h"
#include "minic/Diagnostic.h"
#include "minic/Lexer.h"

#include "llvm/Support/ErrorHandling.h"

#include <string>
#include <utility>
#include <vector>

using namespace minic;

//===----------------------------------------------------------------------===//
// Helpers
//===----------------------------------------------------------------------===//

/// Binding strength of a binary operator token, or 0 if it is not one.
/// Must match the precedence table in docs/grammar.md.
static int getBinaryPrecedence(tok::TokenKind Kind) {
  switch (Kind) {
  case tok::star:
  case tok::slash:
    return 6;
  case tok::plus:
  case tok::minus:
    return 5;
  case tok::less:
  case tok::lessequal:
  case tok::greater:
  case tok::greaterequal:
    return 4;
  case tok::equalequal:
  case tok::exclaimequal:
    return 3;
  case tok::ampamp:
    return 2;
  case tok::pipepipe:
    return 1;
  default:
    return 0;
  }
}

static BinaryOp getBinaryOp(tok::TokenKind Kind) {
  switch (Kind) {
  case tok::star:
    return BinaryOp::Mul;
  case tok::slash:
    return BinaryOp::Div;
  case tok::plus:
    return BinaryOp::Add;
  case tok::minus:
    return BinaryOp::Sub;
  case tok::less:
    return BinaryOp::Lt;
  case tok::lessequal:
    return BinaryOp::Le;
  case tok::greater:
    return BinaryOp::Gt;
  case tok::greaterequal:
    return BinaryOp::Ge;
  case tok::equalequal:
    return BinaryOp::Eq;
  case tok::exclaimequal:
    return BinaryOp::Ne;
  case tok::ampamp:
    return BinaryOp::And;
  case tok::pipepipe:
    return BinaryOp::Or;
  default:
    llvm_unreachable("not a binary operator");
  }
}

/// Describes what the parser wanted, e.g. "')'" or "identifier".
static std::string describeKind(tok::TokenKind Kind) {
  if (const char *Spelling = tok::getPunctuatorSpelling(Kind))
    return "'" + std::string(Spelling) + "'";
  if (const char *Spelling = tok::getKeywordSpelling(Kind))
    return "'" + std::string(Spelling) + "'";
  switch (Kind) {
  case tok::identifier:
    return "identifier";
  case tok::integer_literal:
    return "integer literal";
  case tok::eof:
    return "end of file";
  default:
    return tok::getTokenName(Kind);
  }
}

/// Describes the token actually found, e.g. "identifier 'x'" or "';'".
static std::string describeToken(const Token &T) {
  switch (T.getKind()) {
  case tok::eof:
    return "end of file";
  case tok::identifier:
    return "identifier '" + T.getText().str() + "'";
  case tok::integer_literal:
    return "integer literal '" + T.getText().str() + "'";
  default:
    return "'" + T.getText().str() + "'";
  }
}

//===----------------------------------------------------------------------===//
// Token handling
//===----------------------------------------------------------------------===//

Parser::Parser(Lexer &Lex, DiagnosticEngine &Diags) : Lex(Lex), Diags(Diags) {
  Tok = lexSkippingUnknown();
  NextTok = lexSkippingUnknown();
  PrevTokenEnd = Tok.getText().data();
}

Token Parser::lexSkippingUnknown() {
  // The lexer has already reported every unknown token; skipping them here
  // avoids a second, confusing error from the parser for the same problem.
  Token T = Lex.lex();
  while (T.is(tok::unknown))
    T = Lex.lex();
  return T;
}

void Parser::consume() {
  PrevTokenEnd = Tok.getText().end();
  Tok = NextTok;
  NextTok = lexSkippingUnknown();
}

bool Parser::consumeIf(tok::TokenKind Kind) {
  if (Tok.isNot(Kind))
    return false;
  consume();
  return true;
}

bool Parser::expect(tok::TokenKind Kind) {
  if (consumeIf(Kind))
    return true;

  if (Kind == tok::semi) {
    // Point just after the previous token, where the ';' is missing, rather
    // than at the next token, which is often on the following line.
    Diags.error(llvm::SMLoc::getFromPointer(PrevTokenEnd), "expected ';'");
    return false;
  }

  Diags.error(Tok.getLocation(), "expected " + describeKind(Kind) + ", found " +
                                     describeToken(Tok));
  return false;
}

std::optional<Token> Parser::expectIdentifier() {
  Token Name = Tok;
  if (!expect(tok::identifier))
    return std::nullopt;
  return Name;
}

//===----------------------------------------------------------------------===//
// Error recovery
//===----------------------------------------------------------------------===//

void Parser::skipToStatementEnd() {
  unsigned BraceDepth = 0;
  while (Tok.isNot(tok::eof) && Tok.isNot(tok::kw_fn)) {
    switch (Tok.getKind()) {
    case tok::l_brace:
      ++BraceDepth;
      break;
    case tok::r_brace:
      if (BraceDepth == 0)
        return; // Leave the '}' for the enclosing block.
      if (--BraceDepth == 0) {
        consume(); // Skipped a whole nested block, e.g. a broken if's body.
        return;
      }
      break;
    case tok::semi:
      if (BraceDepth == 0) {
        consume();
        return;
      }
      break;
    default:
      break;
    }
    consume();
  }
}

void Parser::skipToNextFunction() {
  while (Tok.isNot(tok::eof) && Tok.isNot(tok::kw_fn))
    consume();
}

//===----------------------------------------------------------------------===//
// Declarations
//===----------------------------------------------------------------------===//

std::unique_ptr<Program> Parser::parseProgram() {
  auto Prog = std::make_unique<Program>();
  while (Tok.isNot(tok::eof)) {
    if (Tok.is(tok::kw_let)) {
      Diags.error(Tok.getLocation(), "global variables are not supported; "
                                     "declare variables inside a function");
      skipToNextFunction();
      continue;
    }
    if (Tok.isNot(tok::kw_fn)) {
      Diags.error(Tok.getLocation(),
                  "expected 'fn' at top level, found " + describeToken(Tok));
      skipToNextFunction();
      continue;
    }
    if (std::unique_ptr<FunctionDecl> Fn = parseFunction())
      Prog->addFunction(std::move(Fn));
    else
      skipToNextFunction();
  }
  return Prog;
}

std::unique_ptr<FunctionDecl> Parser::parseFunction() {
  consume(); // 'fn'

  std::optional<Token> Name = expectIdentifier();
  if (!Name || !expect(tok::l_paren))
    return nullptr;

  std::vector<ParamDecl> Params;
  if (Tok.isNot(tok::r_paren)) {
    do {
      std::optional<ParamDecl> Param = parseParam();
      if (!Param)
        return nullptr;
      Params.push_back(*Param);
    } while (consumeIf(tok::comma));
  }
  if (!expect(tok::r_paren))
    return nullptr;

  Type ReturnType = Type::Void;
  if (consumeIf(tok::arrow)) {
    std::optional<Type> Ty = parseType();
    if (!Ty)
      return nullptr;
    ReturnType = *Ty;
  }

  std::unique_ptr<BlockStmt> Body = parseBlock();
  if (!Body)
    return nullptr;

  return std::make_unique<FunctionDecl>(Name->getText(), Name->getLocation(),
                                        std::move(Params), ReturnType,
                                        std::move(Body));
}

std::optional<ParamDecl> Parser::parseParam() {
  std::optional<Token> Name = expectIdentifier();
  if (!Name || !expect(tok::colon))
    return std::nullopt;
  std::optional<Type> Ty = parseType();
  if (!Ty)
    return std::nullopt;
  return ParamDecl{Name->getText(), Name->getLocation(), *Ty};
}

std::optional<Type> Parser::parseType() {
  if (consumeIf(tok::kw_int))
    return Type::Int;
  if (consumeIf(tok::kw_bool))
    return Type::Bool;
  Diags.error(Tok.getLocation(), "expected type, found " + describeToken(Tok));
  return std::nullopt;
}

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

std::unique_ptr<BlockStmt> Parser::parseBlock() {
  llvm::SMLoc Loc = Tok.getLocation();
  if (!expect(tok::l_brace))
    return nullptr;

  std::vector<std::unique_ptr<Stmt>> Body;
  // A 'fn' inside a block almost always means a missing '}'.
  while (Tok.isNot(tok::r_brace) && Tok.isNot(tok::eof) &&
         Tok.isNot(tok::kw_fn)) {
    if (std::unique_ptr<Stmt> S = parseStatement())
      Body.push_back(std::move(S));
    else
      skipToStatementEnd();
  }

  // If the '}' is missing, the error is reported but the block is kept.
  expect(tok::r_brace);
  return std::make_unique<BlockStmt>(std::move(Body), Loc);
}

std::unique_ptr<Stmt> Parser::parseStatement() {
  switch (Tok.getKind()) {
  case tok::kw_let:
    return parseLetStmt();
  case tok::kw_if:
    return parseIfStmt();
  case tok::kw_while:
    return parseWhileStmt();
  case tok::kw_return:
    return parseReturnStmt();
  case tok::l_brace:
    return parseBlock();
  case tok::identifier:
    if (NextTok.is(tok::equal))
      return parseAssignStmt();
    break;
  default:
    break;
  }
  return parseExprStmt();
}

std::unique_ptr<Stmt> Parser::parseLetStmt() {
  llvm::SMLoc Loc = Tok.getLocation();
  consume(); // 'let'

  std::optional<Token> Name = expectIdentifier();
  if (!Name || !expect(tok::colon))
    return nullptr;
  std::optional<Type> Ty = parseType();
  if (!Ty || !expect(tok::equal))
    return nullptr;
  std::unique_ptr<Expr> Init = parseExpr();
  if (!Init)
    return nullptr;

  // A missing ';' is reported, but the statement is still kept.
  expect(tok::semi);
  return std::make_unique<LetStmt>(Name->getText(), Name->getLocation(), *Ty,
                                   std::move(Init), Loc);
}

std::unique_ptr<Stmt> Parser::parseAssignStmt() {
  Token Name = Tok;
  consume(); // identifier
  consume(); // '='

  std::unique_ptr<Expr> Value = parseExpr();
  if (!Value)
    return nullptr;
  expect(tok::semi);
  return std::make_unique<AssignStmt>(Name.getText(), std::move(Value),
                                      Name.getLocation());
}

std::unique_ptr<Stmt> Parser::parseIfStmt() {
  llvm::SMLoc Loc = Tok.getLocation();
  consume(); // 'if'

  if (!expect(tok::l_paren))
    return nullptr;
  std::unique_ptr<Expr> Cond = parseExpr();
  if (!Cond || !expect(tok::r_paren))
    return nullptr;
  std::unique_ptr<BlockStmt> Then = parseBlock();
  if (!Then)
    return nullptr;

  std::unique_ptr<Stmt> Else;
  if (consumeIf(tok::kw_else)) {
    if (Tok.is(tok::kw_if))
      Else = parseIfStmt();
    else
      Else = parseBlock();
    if (!Else)
      return nullptr;
  }

  return std::make_unique<IfStmt>(std::move(Cond), std::move(Then),
                                  std::move(Else), Loc);
}

std::unique_ptr<Stmt> Parser::parseWhileStmt() {
  llvm::SMLoc Loc = Tok.getLocation();
  consume(); // 'while'

  if (!expect(tok::l_paren))
    return nullptr;
  std::unique_ptr<Expr> Cond = parseExpr();
  if (!Cond || !expect(tok::r_paren))
    return nullptr;
  std::unique_ptr<BlockStmt> Body = parseBlock();
  if (!Body)
    return nullptr;

  return std::make_unique<WhileStmt>(std::move(Cond), std::move(Body), Loc);
}

std::unique_ptr<Stmt> Parser::parseReturnStmt() {
  llvm::SMLoc Loc = Tok.getLocation();
  consume(); // 'return'

  std::unique_ptr<Expr> Value;
  if (Tok.isNot(tok::semi)) {
    Value = parseExpr();
    if (!Value)
      return nullptr;
  }
  expect(tok::semi);
  return std::make_unique<ReturnStmt>(std::move(Value), Loc);
}

std::unique_ptr<Stmt> Parser::parseExprStmt() {
  llvm::SMLoc Loc = Tok.getLocation();
  std::unique_ptr<Expr> E = parseExpr();
  if (!E)
    return nullptr;
  expect(tok::semi);
  return std::make_unique<ExprStmt>(std::move(E), Loc);
}

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

std::unique_ptr<Expr> Parser::parseExpr() { return parseBinaryExpr(1); }

/// Precedence climbing. Parses a unary operand, then keeps absorbing binary
/// operators whose precedence is at least MinPrecedence. The right operand is
/// parsed with Precedence + 1, which makes every operator left-associative.
std::unique_ptr<Expr> Parser::parseBinaryExpr(int MinPrecedence) {
  std::unique_ptr<Expr> LHS = parseUnaryExpr();
  if (!LHS)
    return nullptr;

  while (true) {
    int Precedence = getBinaryPrecedence(Tok.getKind());
    if (Precedence < MinPrecedence)
      return LHS;

    BinaryOp Op = getBinaryOp(Tok.getKind());
    llvm::SMLoc OpLoc = Tok.getLocation();
    consume();

    std::unique_ptr<Expr> RHS = parseBinaryExpr(Precedence + 1);
    if (!RHS)
      return nullptr;
    LHS =
        std::make_unique<BinaryExpr>(Op, std::move(LHS), std::move(RHS), OpLoc);
  }
}

std::unique_ptr<Expr> Parser::parseUnaryExpr() {
  if (Tok.isOneOf(tok::minus, tok::exclaim)) {
    UnaryOp Op = Tok.is(tok::minus) ? UnaryOp::Neg : UnaryOp::Not;
    llvm::SMLoc Loc = Tok.getLocation();
    consume();
    std::unique_ptr<Expr> Operand = parseUnaryExpr();
    if (!Operand)
      return nullptr;
    return std::make_unique<UnaryExpr>(Op, std::move(Operand), Loc);
  }
  return parsePrimaryExpr();
}

std::unique_ptr<Expr> Parser::parsePrimaryExpr() {
  switch (Tok.getKind()) {
  case tok::integer_literal: {
    std::unique_ptr<Expr> E =
        std::make_unique<IntegerLiteralExpr>(Tok.getText(), Tok.getLocation());
    consume();
    return E;
  }
  case tok::kw_true:
  case tok::kw_false: {
    std::unique_ptr<Expr> E = std::make_unique<BoolLiteralExpr>(
        Tok.is(tok::kw_true), Tok.getLocation());
    consume();
    return E;
  }
  case tok::identifier: {
    if (NextTok.is(tok::l_paren))
      return parseCallExpr();
    std::unique_ptr<Expr> E =
        std::make_unique<VarRefExpr>(Tok.getText(), Tok.getLocation());
    consume();
    return E;
  }
  case tok::l_paren: {
    consume(); // '('
    std::unique_ptr<Expr> E = parseExpr();
    if (!E || !expect(tok::r_paren))
      return nullptr;
    return E; // Parentheses only group; they need no node of their own.
  }
  default:
    Diags.error(Tok.getLocation(),
                "expected expression, found " + describeToken(Tok));
    return nullptr;
  }
}

std::unique_ptr<Expr> Parser::parseCallExpr() {
  Token Callee = Tok;
  consume(); // identifier
  consume(); // '('

  std::vector<std::unique_ptr<Expr>> Args;
  if (Tok.isNot(tok::r_paren)) {
    do {
      std::unique_ptr<Expr> Arg = parseExpr();
      if (!Arg)
        return nullptr;
      Args.push_back(std::move(Arg));
    } while (consumeIf(tok::comma));
  }
  if (!expect(tok::r_paren))
    return nullptr;

  return std::make_unique<CallExpr>(Callee.getText(), std::move(Args),
                                    Callee.getLocation());
}