package com.niyor.tupal;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
import java.util.ArrayList;

class Tupal{
   
  private static final Interpreter interpreter = new Interpreter();
  static boolean hadError = false;
  static boolean hadRuntimeError = false;
  static UnicodeToToken translate= new UnicodeToToken();
 public static void main(String[] args) throws IOException {
   if(args.length > 1 ){
    System.out.println("usage : [niyor] script");
    System.exit(64);
  }
  else if(args.length == 1){
    runfile(args[0]);
  } else {
    runPrompt();
  }

 }
 private static void runfile(String path) throws IOException{
  byte[] bytes = Files.readAllBytes(Paths.get(path));
  String file = new String(bytes, StandardCharsets.UTF_8);
  List<String> source = StringToUnicode(file);
  System.out.println(source);
  run(source);
  if (hadError) System.exit(65);
  if (hadRuntimeError) System.exit(70);

 }

 private static void runPrompt() throws IOException{
 InputStreamReader input = new InputStreamReader(System.in);
 BufferedReader reader = new BufferedReader(input);
  while(true){
      System.out.print("> ");
      String line = reader.readLine();
      if(line == null) break;
      List<String> text = StringToUnicode(line);
      run(text);
  }
    System.exit(68);
 }
 private static void run(List<String> source){
  Scanner scanner = new Scanner(source);
  List<Token> tokens = scanner.scanTokens();
  for(Token token : tokens){
    System.out.println(token);
  }
  Parser parser = new Parser(tokens);
  List<Stmt> statements = parser.parse();
  if(hadError) return;
  interpreter.interpret(statements);
 }

private static List<String> StringToUnicode(String file) {
    List<String> words = new ArrayList<>();
    StringBuilder word = new StringBuilder();

    for (char lexeme : file.toCharArray()) {
        int code = lexeme;
        String hex = String.format("%04X", code);

        // All delimiters, including "
        if (lexeme == ';' || lexeme == '\n' ||
            lexeme == '(' || lexeme == ')' ||
            lexeme == '{' || lexeme == '}' ||
            lexeme == '"') {

            if (word.length() > 0) {
                words.add(word.toString().trim());
                word.setLength(0);
            }
            words.add(hex);
        }
        else if (lexeme == ' ') {
            if (word.length() > 0) {
                words.add(word.toString().trim());
                word.setLength(0);
            }
        }
        else {
            word.append(hex).append(" ");
        }
    }

    if (word.length() > 0) {
        words.add(word.toString().trim());
    }

    return words;
}


static void error(int line, String message) {
report(line, "", message);
}
private static void report(int line, String where,
String message) {
System.err.println(
"[line " + line + "] Error" + where + ": " + message);
hadError = true;
}


static void error(Token token, String message) {
    if (token.type == TokenType.EOF) {
        report(token.line, " at end", message);
    } else {
        String display;

        // Check if it's a full word (multi-hex string)
        if (!token.lexeme.matches("^[0-9A-Fa-f]{4}$")) {
            // Multi-hex or invalid: convert entire thing
            display = translate.hexToAssamese(token.lexeme);
        } else {
            // Single hex code: convert normally
            try {
                int codePoint = Integer.parseInt(token.lexeme, 16);
                display = String.valueOf((char) codePoint);
            } catch (NumberFormatException e) {
                display = token.lexeme; // fallback
            }
        }

        report(token.line, " at '" + display + "'", message);
    }
}

static void runtimeError(RuntimeError error) {
System.err.println(error.getMessage() +
"\n[line " + error.token.line + "]");
hadRuntimeError = true;
}



}
