//===- Token.h - minic token representation -----------------------*- C++
//-*-===//
//
// A Token is a kind plus the exact source text it covers. The text points
// into the source buffer owned by llvm::SourceMgr, so tokens are cheap to
// copy but must not outlive that SourceMgr.
//
//===----------------------------------------------------------------------===//

#ifndef MINIC_TOKEN_H
#define MINIC_TOKEN_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SMLoc.h"

namespace minic {

namespace tok {

enum TokenKind : unsigned short {
#define TOK(ID) ID,
#include "minic/TokenKinds.def"
  NUM_TOKENS
};

/// Returns the enumerator name of a kind, e.g. "l_paren" or "kw_fn".
const char *getTokenName(TokenKind Kind);

/// Returns the source spelling of a punctuator, e.g. "(", or nullptr.
const char *getPunctuatorSpelling(TokenKind Kind);

/// Returns the source spelling of a keyword, e.g. "fn", or nullptr.
const char *getKeywordSpelling(TokenKind Kind);

} // namespace tok

class Token {
public:
  Token() = default;
  Token(tok::TokenKind Kind, llvm::StringRef Text) : Kind(Kind), Text(Text) {}

  tok::TokenKind getKind() const { return Kind; }
  bool is(tok::TokenKind K) const { return Kind == K; }
  bool isNot(tok::TokenKind K) const { return Kind != K; }
  template <typename... Kinds> bool isOneOf(Kinds... Ks) const {
    return (is(Ks) || ...);
  }

  llvm::StringRef getText() const { return Text; }
  llvm::SMLoc getLocation() const {
    return llvm::SMLoc::getFromPointer(Text.data());
  }

private:
  tok::TokenKind Kind = tok::unknown;
  llvm::StringRef Text;
};

} // namespace minic

#endif // MINIC_TOKEN_H