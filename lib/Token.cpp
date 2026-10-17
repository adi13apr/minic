//===- Token.cpp - minic token kind helpers -------------------------------===//

#include "minic/Token.h"

#include <cassert>

using namespace minic;

static const char *const TokenNames[] = {
#define TOK(ID) #ID,
#include "minic/TokenKinds.def"
};

const char *tok::getTokenName(TokenKind Kind) {
  assert(Kind < NUM_TOKENS && "invalid token kind");
  return TokenNames[Kind];
}

const char *tok::getPunctuatorSpelling(TokenKind Kind) {
  switch (Kind) {
#define PUNCTUATOR(ID, SPELLING)                                               \
  case ID:                                                                     \
    return SPELLING;
#include "minic/TokenKinds.def"
  default:
    return nullptr;
  }
}

const char *tok::getKeywordSpelling(TokenKind Kind) {
  switch (Kind) {
#define KEYWORD(NAME)                                                          \
  case kw_##NAME:                                                              \
    return #NAME;
#include "minic/TokenKinds.def"
  default:
    return nullptr;
  }
}