# CyberLang Compiler (Modular)

A modular compiler for **CyberLang** built with **Flex, Bison, and C**.

This project follows a clean multi-pass architecture where each pass has one job and all passes operate on the same AST.

## Architecture

```
yyparse()         -> AST construction only       (parser.y + lexer.l + ast.c)
semantic_check()  -> type checks + declarations  (semantic.c + symtab.c)
tac_generate()    -> TAC to stdout               (tac.c)
exec_program()    -> runtime interpretation      (exec.c)
```

## Project Structure

```
projectr/
├── include/
│   ├── value.h       # Shared type/value definitions
│   ├── ast.h         # AST node types and constructors
│   ├── symtab.h      # Symbol table interface
│   ├── functab.h     # Function table interface (placeholder)
│   ├── semantic.h    # Semantic pass interface
│   ├── tac.h         # TAC generation interface
│   └── exec.h        # Runtime execution interface
├── src/
│   ├── main.c        # Orchestrates parse -> semantic -> TAC -> exec
│   ├── lexer.l       # Flex lexer
│   ├── parser.y      # Bison grammar (AST-only actions)
│   ├── ast.c         # AST constructors and cleanup
│   ├── value.c       # Runtime value utilities and casting
│   ├── symtab.c      # Symbol table implementation
│   ├── functab.c     # Function table implementation (placeholder)
│   ├── semantic.c    # Semantic analysis
│   ├── tac.c         # TAC emission from AST
│   └── exec.c        # AST interpreter
├── tests/
│   └── test_01_variables.txt
├── outputs/
├── Makefile
└── README.md
```

## Separation of Concerns

| File | Responsibility | Must NOT |
|------|----------------|----------|
| `parser.y` | Build AST nodes | Modify symbol table / emit TAC / execute program |
| `semantic.c` | Validate declarations and types | Execute code / emit TAC |
| `tac.c` | Emit 3-address-style intermediate code | Change semantic state / execute code |
| `exec.c` | Execute the AST at runtime | Emit TAC |

## Build

### Prerequisites

- `gcc`
- `bison`
- `flex`
- `make`

### Using Make (recommended)

```sh
make
```

### Manual Build

```sh
# 1) Generate parser
bison -d -o src/parser.tab.c src/parser.y

# 2) Generate lexer
flex -o src/lex.yy.c src/lexer.l

# 3) Compile
gcc -Iinclude -Wall -Wextra -std=c11 -o compiler \
	src/parser.tab.c src/lex.yy.c \
	src/main.c src/ast.c src/value.c src/symtab.c src/functab.c \
	src/semantic.c src/tac.c src/exec.c
```

### Clean

```sh
make clean
```

## Run

### Run from stdin

```sh
cat tests/test_01_variables.txt | ./compiler
```

### Run from a file path

```sh
./compiler tests/test_01_variables.txt
```

Compiler output order:
1. TAC lines prefixed with `TAC:`
2. Runtime output from `log`
3. Final status line `Parsing + Semantic OK.`

### Exit Codes

| Code | Meaning |
|------|---------|
| `0` | Success |
| `1` | Parse error |
| `2` | Semantic error |

## Test

### Single test

```sh
cat tests/test_01_variables.txt | ./compiler
```

### Current test inventory

| File | What it checks |
|------|----------------|
| `test_01_variables.txt` | Basic declarations, assignments, output |

## Language Reference (Current)

### Type keywords

| CyberLang | Internal type |
|-----------|----------------|
| `auth` | `int` |
| `secure` | `float` |
| `packet` | `long long` |

Additional notes:
- String literals are supported in expressions/output.
- Boolean values are produced by relational expressions (`>`, `<`, `==`, `!=`).

### Keywords

| Keyword | Purpose |
|---------|---------|
| `auth` | int declaration type |
| `secure` | float declaration type |
| `packet` | long long declaration type |
| `scan` | input into variable |
| `log` | output expression |
| `firewall` | if |
| `breach` | else |
| `monitor` | while |
| `iterate` | for |
| `exitsec` | return |
| `abort` | break |

### Operators

- Arithmetic: `+`, `-`, `*`, `/`
- Relational: `>`, `<`, `==`, `!=`
- Assignment: `=`

### Syntax Examples

#### Variable declaration

```txt
auth x;
secure y;
packet p;
```

#### Assignment

```txt
x = 10;
y = 3.14;
```

#### Input / output

```txt
scan x;
log x;
log "Hello";
```

#### Conditional

```txt
firewall (x > 0) {
	log x;
} breach {
	log 0;
}
```

#### While loop

```txt
monitor (x > 0) {
	x = x - 1;
}
```

#### For loop

```txt
iterate (x = 0; x < 5; x = x + 1) {
	log x;
}
```

#### Return / break

```txt
exitsec 0;
abort;
```

## Notes
- `functab` is scaffolded for future function features.
