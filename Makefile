
# Makefile (Linux friendly)

SRC_DIR := src
BIN_DIR := bin
MAIN_CLASS := com.niyor.tupal.Tupal

SOURCES := $(SRC_DIR)/com/niyor/tupal/Token.java \
		   $(SRC_DIR)/com/niyor/tupal/TokenType.java \
		   $(SRC_DIR)/com/niyor/tupal/Tupal.java \
		   $(SRC_DIR)/com/niyor/tupal/UnicodeToToken.java \
           $(SRC_DIR)/com/niyor/tupal/Scanner.java \
           $(SRC_DIR)/com/niyor/tupal/RuntimeError.java \
           $(SRC_DIR)/com/niyor/tupal/Expr.java \
           $(SRC_DIR)/com/niyor/tupal/Stmt.java \
           $(SRC_DIR)/com/niyor/tupal/Environment.java \
           $(SRC_DIR)/com/niyor/tupal/Parser.java \
           $(SRC_DIR)/com/niyor/tupal/Interpreter.java \
           # Add other .java files here manually

all: compile

compile:
	@mkdir -p $(BIN_DIR)
	javac -d $(BIN_DIR) $(SOURCES)

run: compile
	java -cp $(BIN_DIR) $(MAIN_CLASS) $(file)

clean:
	rm -rf $(BIN_DIR)

