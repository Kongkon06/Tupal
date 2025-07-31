package com.niyor.tupal;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
public class Tupal {

  private static final Interpreter interpreter = new Interpreter();
  static boolean hadError = false;
  static boolean hadRuntimeError = false;
public static void main(String[] args) throws IOException {
if (args.length > 1) {
System.out.println("Usage: jlox [script]");
System.exit(64);

} else if (args.length == 1) {
runFile(args[0]);

} else {
runPrompt();

}

}

private static void runfile(String path) throws IOException{
  byte[] bytes = Files.readAllBytes(Paths.get(path));
  String file = new String(bytes, StandardCharsets.UTF_8);
  List<String> source = StringToUnicode(file);
  run(source);
  if (hadError) System.exit(65);
  if (hadRuntimeError) System.exit(70);

 }


private static void runPrompt() throws IOException {
InputStreamReader input = new InputStreamReader(System.in);
BufferedReader reader = new BufferedReader(input);
for (;;) {
System.out.print("> ");
String line = reader.readLine();
if (line == null) break;
run(line);
hadError = false;
}
}

private static void run(List<String> source) {
Scanner scanner = new Scanner(source);
List<Token> tokens = scanner.scanTokens();
Parser parser = new Parser(tokens);
List<Stmt> statements = parser.parse();
// Stop if there was a syntax error.
if (hadError) return;
interpreter.interpret(statements);
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
report(token.line, " at '" + token.lexeme + "'", message);
}

}

static void runtimeError(RuntimeError error) {
System.err.println(error.getMessage() +
"\n[line " + error.token.line + "]");
hadRuntimeError = true;
}
private static List<String> StringToUnicode(String file) {
    List<String> words = new ArrayList<>();
    StringBuilder word = new StringBuilder();

    for (char lexeme : file.toCharArray()) {
        int code = (int) lexeme;
        String hex = String.format("%04X", code);

        // Handle special characters: ; \n { }
        if (lexeme == ';' || lexeme == '\n' || lexeme == '{' || lexeme == '}') {
            if (word.length() > 0) {
                words.add(word.toString().trim());
                word.setLength(0);
            }
            words.add(hex); // add special character as separate item
        } else if (lexeme == ' ') {
            if (word.length() > 0) {
                words.add(word.toString().trim());
                word.setLength(0);
            }
        } else {
            word.append(hex).append(" ");
        }
    }

    // Add any remaining word
    if (word.length() > 0) {
        words.add(word.toString().trim());
    }

    return words;
}


}
