package com.niyor.tupal;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import static com.niyor.tupal.TokenType.*;
class Scanner {
private final List<String> source;
private final List<Token> tokens = new ArrayList<>();
private int start = 0;
private int current = 0;
private int line = 1;
private static final Map<String, TokenType> assameseKeyword;

    private final UnicodeToToken assm = new UnicodeToToken();
    static {
        assameseKeyword = new HashMap<>();

        assameseKeyword.put("0986 09F0 09C1", AND);
        assameseKeyword.put("09AC 09BF 09A7", CLASS);
        assameseKeyword.put("0985 09A8 09CD 09AF 09A5 09BE", ELSE);
        assameseKeyword.put("09AD 09C1 09B2", FALSE);
        assameseKeyword.put("09DF 09C7 09A4 09BF 09DF 09BE", FOR);
        assameseKeyword.put("09AF 09A6 09BF", IF);
        assameseKeyword.put("09A8 09BE 0987", NIL);
        assameseKeyword.put("0986 09B0", OR);
        assameseKeyword.put("09AB 09B2 09A8", FUN);
        assameseKeyword.put("09AA 09CD 09F0 0995 09BE 09B6", PRINT);
        assameseKeyword.put("09B8 09A4 09CD 09AF", TRUE);
        assameseKeyword.put("099A 09B2 0995", VAR);
        assameseKeyword.put("0996 09BE 09A8 09CD 09A6 09BF", WHILE);
    }

Scanner(List<String> source) {
this.source = source;
}

List<Token> scanTokens() {
while (!isAtEnd()) {
// We are at the beginning of the next lexeme.
start = current;
scanToken();
}
tokens.add(new Token(EOF, "", null, line));
return tokens;
}


    private void scanToken() {
        String character = advance();
        switch (character) {
            case "0028": addToken(LEFT_PAREN); break;
            case "0029": addToken(RIGHT_PAREN); break;
            case "007B": addToken(LEFT_BRACE); break;
            case "007D": addToken(RIGHT_BRACE); break;
            case "002C": addToken(COMMA); break;
            case "002E": addToken(DOT); break;
            case "002D": addToken(MINUS); break;
            case "002B": addToken(PLUS); break;
            case "003B": addToken(SEMICOLON); break;
            case "002A": addToken(STAR); break;
            case "0021 003D": addToken(BANG_EQUAL);break;
            case "0021": addToken(BANG); break;
            case "003D": addToken(EQUAL);break;
            case "003D 003D": addToken(EQUAL_EQUAL);break;
            case "003C": addToken(LESS);break;
            case "003C 003D": addToken(LESS_EQUAL);break;
            case "003E": addToken(GREATER);break;
            case "003E 003D": addToken(GREATER_EQUAL);break;
            case "002F":
                if (match("002F")) {
                    while (!peek().equals("000A") && !isAtEnd()) advance();
                } else {
                    addToken(SLASH);
                }
                break;
            case "0020": case "000D": case "0009":
                break; // Ignore whitespace
            case "000A":
                line++;
                break;
            case "0022":
                string();
                break;
            default:
                if (isDigit(character)) {
                    number();
                } else if (isAlphaNumeric(character)) {
                    identifier();
                }
                break;
        }
    }



    private void identifier() {

        String text = source.get(current-1);
        TokenType type = assameseKeyword.getOrDefault(text, IDENTIFIER);
        addToken(type);
    }

private void string(){
StringBuilder word = new StringBuilder();
int start =current;
while (peek() != "0022" && !isAtEnd()) {
if (peek() == "000A") line++;
word.append(advance());
}
if (isAtEnd()) {
Tupal.error(line, "Unterminated string.");
return;
}
advance();
// Trim the surrounding quotes.
addToken(STRING, word);
}
    private void number() {
        while (isDigit(peek())) advance();

        if (peek().equals("002E") && isDigit(peekNext())) {
            advance(); // consume '.'
            while (isDigit(peek())) advance();
        }

        String literal = assm.getLiteral(source.get(current-1));
        addToken(NUMBER, Double.parseDouble(literal));
    }



private boolean match(String expected) {
if (isAtEnd()) return false;
if (source.get(current) != expected) return false;
current++;
return true;
}

private String peek() {
if (isAtEnd()) return "\0";
return source.get(current);
}

private String peekNext() {
if (current + 1 >= source.size()) return "\0";
return source.get(current + 1);
}


private boolean isAlpha(String c) {
    if (c == null || c.isEmpty()) return false;

    String[] hexCodes = c.trim().split("\\s+");
    if (hexCodes.length == 0 || hexCodes[0].length() != 4) return false;

    int code;
    try {
        code = Integer.parseInt(hexCodes[0], 16);
    } catch (NumberFormatException e) {
        return false;
    }

    return (code >= 0x0985 && code <= 0x09B9) || // অ - হ
           code == 0x09CE ||                    // ট্
           (code >= 0x09DC && code <= 0x09DF) ||// ড়, ঢ়, য়
           (code >= 0x09BE && code <= 0x09CC) ||// vowel signs
           code == 0x09D7 ||                    // au length mark
           code == 0x09F0 || code == 0x09F1;    // ৰ, ৱ
}


private boolean isAlphaNumeric(String c) {
return isAlpha(c) || isDigit(c);
}


    private boolean isDigit(String c) {
        String[] digits = c.split(" ");
        for(String digit : digits){
         if (digit.length() != 4) return false;
        int code = Integer.parseInt(digit, 16);
        if (code < 0x09E6 || code > 0x09EF) {
            return false; // not an Assamese digit
        }
        }
        return true;
       }

    private boolean isAtEnd() {
        return current >= source.size();
    }

private String advance() {
return source.get(current++);
}
private void addToken(TokenType type) {
addToken(type, null);
}
    private void addToken(TokenType type, Object literal) {
        String text = assm.fromUnicodeHex(source.get(current-1));
        tokens.add(new Token(type, text, literal, line));
    }

}
