//===- Lexer.cpp - minic lexer --------------------------------------------===//

#include "minic/Lexer.h"
#include "minic/Diagnostic.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"

#include <cstddef>

using namespace minic;

static bool isIdentifierStart(char C) { return llvm::isAlpha(C) || C == '_'; }
static bool isIdentifierChar(char C) { return llvm::isAlnum(C) || C == '_'; }
static llvm::SMLoc getLoc(const char *Ptr) {
  return llvm::SMLoc::getFromPointer(Ptr);
}

Lexer::Lexer(const llvm::SourceMgr &SrcMgr, unsigned BufferID,
             DiagnosticEngine &Diags)
    : Diags(Diags) {
  const llvm::MemoryBuffer *Buffer = SrcMgr.getMemoryBuffer(BufferID);
  BufferEnd = Buffer->getBufferEnd();
  CurPtr = Buffer->getBufferStart();
}

char Lexer::peek(unsigned Offset) const {
  return static_cast<std::size_t>(BufferEnd - CurPtr) > Offset ? CurPtr[Offset]
                                                               : '\0';
}

void Lexer::skipWhitespaceAndComments() {
  while (CurPtr != BufferEnd) {
    if (llvm::isSpace(*CurPtr)) {
      ++CurPtr;
      continue;
    }
    if (peek() == '/' && peek(1) == '/') {
      while (CurPtr != BufferEnd && *CurPtr != '\n')
        ++CurPtr;
      continue;
    }
    return;
  }
}

Token Lexer::makeToken(tok::TokenKind Kind, const char *Start) const {
  return Token(Kind, llvm::StringRef(Start, CurPtr - Start));
}

tok::TokenKind Lexer::selectIfNext(char Next, tok::TokenKind IfMatch,
                                   tok::TokenKind Otherwise) {
  if (peek() != Next)
    return Otherwise;
  ++CurPtr;
  return IfMatch;
}

Token Lexer::lex() {
  skipWhitespaceAndComments();

  const char *Start = CurPtr;
  if (CurPtr == BufferEnd)
    return makeToken(tok::eof, Start);

  char C = *CurPtr++;
  if (isIdentifierStart(C))
    return lexIdentifierOrKeyword(Start);
  if (llvm::isDigit(C))
    return lexNumber(Start);

  switch (C) {
  case '(':
    return makeToken(tok::l_paren, Start);
  case ')':
    return makeToken(tok::r_paren, Start);
  case '{':
    return makeToken(tok::l_brace, Start);
  case '}':
    return makeToken(tok::r_brace, Start);
  case ';':
    return makeToken(tok::semi, Start);
  case ':':
    return makeToken(tok::colon, Start);
  case ',':
    return makeToken(tok::comma, Start);
  case '+':
    return makeToken(tok::plus, Start);
  case '*':
    return makeToken(tok::star, Start);
  case '/':
    return makeToken(tok::slash, Start);
  case '-':
    return makeToken(selectIfNext('>', tok::arrow, tok::minus), Start);
  case '=':
    return makeToken(selectIfNext('=', tok::equalequal, tok::equal), Start);
  case '!':
    return makeToken(selectIfNext('=', tok::exclaimequal, tok::exclaim), Start);
  case '<':
    return makeToken(selectIfNext('=', tok::lessequal, tok::less), Start);
  case '>':
    return makeToken(selectIfNext('=', tok::greaterequal, tok::greater), Start);
  case '&':
    if (peek() == '&') {
      ++CurPtr;
      return makeToken(tok::ampamp, Start);
    }
    Diags.error(getLoc(Start), "unexpected '&'; did you mean '&&'?");
    return makeToken(tok::unknown, Start);
  case '|':
    if (peek() == '|') {
      ++CurPtr;
      return makeToken(tok::pipepipe, Start);
    }
    Diags.error(getLoc(Start), "unexpected '|'; did you mean '||'?");
    return makeToken(tok::unknown, Start);
  default:
    break;
  }

  reportUnexpectedCharacter(Start);
  return makeToken(tok::unknown, Start);
}

Token Lexer::lexIdentifierOrKeyword(const char *Start) {
  while (isIdentifierChar(peek()))
    ++CurPtr;

  llvm::StringRef Text(Start, CurPtr - Start);
  tok::TokenKind Kind = llvm::StringSwitch<tok::TokenKind>(Text)
#define KEYWORD(NAME) .Case(#NAME, tok::kw_##NAME)
#include "minic/TokenKinds.def"
                            .Default(tok::identifier);
  return makeToken(Kind, Start);
}

Token Lexer::lexNumber(const char *Start) {
  while (llvm::isDigit(peek()))
    ++CurPtr;

  // Reject things like "12abc" as one bad token instead of a number
  // followed by an identifier. Range checking is done later, in Sema.
  if (isIdentifierStart(peek())) {
    const char *SuffixStart = CurPtr;
    while (isIdentifierChar(peek()))
      ++CurPtr;
    Diags.error(getLoc(SuffixStart),
                "invalid suffix '" +
                    llvm::StringRef(SuffixStart, CurPtr - SuffixStart) +
                    "' on integer literal");
    return makeToken(tok::unknown, Start);
  }
  return makeToken(tok::integer_literal, Start);
}

void Lexer::reportUnexpectedCharacter(const char *Start) {
  auto Byte = static_cast<unsigned char>(*Start);
  if (Byte >= 0x80) {
    // Skip the rest of this UTF-8 sequence so one character gives one error.
    while (CurPtr != BufferEnd &&
           (static_cast<unsigned char>(*CurPtr) & 0xC0) == 0x80)
      ++CurPtr;
    Diags.error(getLoc(Start), "non-ASCII characters are not supported");
    return;
  }
  if (llvm::isPrint(*Start)) {
    Diags.error(getLoc(Start),
                "unexpected character '" + llvm::StringRef(Start, 1) + "'");
    return;
  }
  Diags.error(getLoc(Start),
              "unexpected character with value 0x" + llvm::utohexstr(Byte));
}