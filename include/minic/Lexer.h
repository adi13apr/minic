//===- Lexer.h - minic lexer --------------------------------------*- C++
//-*-===//
//
// Turns a source buffer into tokens on demand: each call to lex() returns the
// next token. Lexical errors are reported through the DiagnosticEngine and
// produce a tok::unknown token, so lexing always continues to the end.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_LEXER_H
#define MINIC_LEXER_H

#include "minic/Token.h"

namespace llvm {
class SourceMgr;
} // namespace llvm

namespace minic {

class DiagnosticEngine;

class Lexer {
public:
  Lexer(const llvm::SourceMgr &SrcMgr, unsigned BufferID,
        DiagnosticEngine &Diags);

  /// Returns the next token. After the end of the buffer, always returns eof.
  Token lex();

private:
  char peek(unsigned Offset = 0) const;
  void skipWhitespaceAndComments();
  Token lexIdentifierOrKeyword(const char *Start);
  Token lexNumber(const char *Start);
  tok::TokenKind selectIfNext(char Next, tok::TokenKind IfMatch,
                              tok::TokenKind Otherwise);
  void reportUnexpectedCharacter(const char *Start);
  Token makeToken(tok::TokenKind Kind, const char *Start) const;

  DiagnosticEngine &Diags;
  const char *BufferEnd;
  const char *CurPtr;
};

} // namespace minic

#endif // MINIC_LEXER_H