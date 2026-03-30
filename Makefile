CC = gcc
BISON = bison
FLEX = flex

CFLAGS = -Iinclude -Wall -Wextra -std=c11
TARGET = compiler

SRCS = src/main.c src/ast.c src/value.c src/symtab.c src/functab.c src/semantic.c src/tac.c src/exec.c
GEN_SRCS = src/parser.tab.c src/lex.yy.c

all: $(TARGET)

src/parser.tab.c src/parser.tab.h: src/parser.y include/ast.h include/value.h
	$(BISON) -d -o src/parser.tab.c src/parser.y

src/lex.yy.c: src/lexer.l src/parser.tab.h
	$(FLEX) -o src/lex.yy.c src/lexer.l

$(TARGET): src/parser.tab.c src/lex.yy.c $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) src/parser.tab.c src/lex.yy.c $(SRCS)

clean:
	rm -f $(TARGET) src/parser.tab.c src/parser.tab.h src/lex.yy.c

test: $(TARGET)
	cat tests/test_01_variables.txt | ./$(TARGET)

.PHONY: all clean test
