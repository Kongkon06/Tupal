

//> Scanning on Demand scanner-c
#include <stdio.h>
#include <string.h>
#include <wctype.h>

#include "common.h"
#include "scanner.h"

typedef struct {
  const wchar_t* start;
  const wchar_t* current;
  int line;
} Scanner;

Scanner scanner;
//> init-scanner
void initScanner(const wchar_t* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}
static bool isAlpha(wchar_t c) {
  return iswalpha(c) || c == L'_';
}

static bool isDigit(wchar_t c) {
  return iswdigit(c);
}
static bool isAtEnd() {
  return *scanner.current == L'\0';
}

static wchar_t advance() {
  scanner.current++;
  return scanner.current[-1];
}

static wchar_t peek() {
  return *scanner.current;
}

static wchar_t peekNext() {
  if (isAtEnd()) return L'\0';
  return scanner.current[1];
}
static bool match(wchar_t expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++;
  return true;
}
static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}
//< make-token
//> error-token
static Token errorToken(const wchar_t* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

static void skipWhitespace() {
  for (;;) {
    wchar_t c = peek();
    switch (c) {
      case L' ':
      case L'\r':
      case L'\t':
        advance();
        break;

      case L'\n':
        scanner.line++;
        advance();
        break;

      case L'/':
        if (peekNext() == L'/') {
          while (peek() != L'\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;

      default:
        return;
    }
  }
}

static TokenType checkKeyword(int start, int length,
    const wchar_t* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      wmemcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}
//< check-keyword
//> identifier-type
static TokenType identifierType() {
//> keywords
  switch (scanner.start[0]) {
    case L'a': return checkKeyword(1, 2, L"nd", TOKEN_AND);
    case L'c': return checkKeyword(1, 4, L"lass", TOKEN_CLASS);
    case L'e': return checkKeyword(1, 3, L"lse", TOKEN_ELSE);
//> keyword-f
    case L'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case L'a': return checkKeyword(2, 3, L"lse", TOKEN_FALSE);
          case L'o': return checkKeyword(2, 1, L"r", TOKEN_FOR);
          case L'u': return checkKeyword(2, 1, L"n", TOKEN_FUN);
        }
      }
      break;
//< keyword-f
    case L'i': return checkKeyword(1, 1, L"f", TOKEN_IF);
    case L'n': return checkKeyword(1, 2, L"il", TOKEN_NIL);
    case L'o': return checkKeyword(1, 1, L"r", TOKEN_OR);
    case L'p': return checkKeyword(1, 4, L"rint", TOKEN_PRINT);
    case L'r': return checkKeyword(1, 5, L"eturn", TOKEN_RETURN);
    case L's': return checkKeyword(1, 4, L"uper", TOKEN_SUPER);
//> keyword-t
    case L't':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case L'h': return checkKeyword(2, 2, L"is", TOKEN_THIS);
          case L'r': return checkKeyword(2, 2, L"ue", TOKEN_TRUE);
        }
      }
      break;
//< keyword-t
    case L'v': return checkKeyword(1, 2, L"ar", TOKEN_VAR);
    case L'w': return checkKeyword(1, 4, L"hile", TOKEN_WHILE);
  }

//< keywords
  return TOKEN_IDENTIFIER;
}
//< identifier-type
//> identifier
static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}
//< identifier
//> number
static Token number() {
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume the ".".
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TOKEN_NUMBER);
}
//< number
//> string
static Token string() {
  while (peek() != L'"' && !isAtEnd()) {
    if (peek() == L'\n') scanner.line++;
    advance();
  }

  if (isAtEnd()) return errorToken(L"Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}
//< string
//> scan-token
Token scanToken() {
//> call-skip-whitespace
  skipWhitespace();
//< call-skip-whitespace
  scanner.start = scanner.current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);
//> scan-char
  
 wchar_t c = advance();
//> scan-identifier
  if (isAlpha(c)) return identifier();
//< scan-identifier
//> scan-number
  if (isDigit(c)) return number();
//< scan-number

  switch (c) {
    case L'(': return makeToken(TOKEN_LEFT_PAREN);
    case L')': return makeToken(TOKEN_RIGHT_PAREN);
    case L'{': return makeToken(TOKEN_LEFT_BRACE);
    case L'}': return makeToken(TOKEN_RIGHT_BRACE);
    case L';': return makeToken(TOKEN_SEMICOLON);
    case L',': return makeToken(TOKEN_COMMA);
    case L'.': return makeToken(TOKEN_DOT);
    case L'-': return makeToken(TOKEN_MINUS);
    case L'+': return makeToken(TOKEN_PLUS);
    case L'/': return makeToken(TOKEN_SLASH);
    case L'*': return makeToken(TOKEN_STAR);
//> two-char
    case L'!':
      return makeToken(
          match(L'=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case L'=':
      return makeToken(
          match(L'=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case L'<':
      return makeToken(
          match(L'=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case L'>':
      return makeToken(
          match(L'=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
//< two-char
//> scan-string
    case L'"': return string();
//< scan-string
  }
//< scan-char

  return errorToken(L"Unexpected character.");
}
//< scan-token
