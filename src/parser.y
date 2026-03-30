%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

StmtList* g_program_ast = NULL;

extern int yylex(void);
extern int yylineno;

void yyerror(const char* s);

static char* strip_quotes(const char* raw) {
    size_t n;
    char* out;

    if (!raw) {
        out = (char*)malloc(1);
        out[0] = '\0';
        return out;
    }

    n = strlen(raw);
    if (n >= 2 && raw[0] == '"' && raw[n - 1] == '"') {
        out = (char*)malloc(n - 1);
        memcpy(out, raw + 1, n - 2);
        out[n - 2] = '\0';
        return out;
    }

    out = (char*)malloc(n + 1);
    memcpy(out, raw, n + 1);
    return out;
}
%}

%union {
    char* str;
    void* ptr;
    int ival;
}

%token AUTH SECURE PACKET SCAN LOG FIREWALL BREACH MONITOR ITERATE EXITSEC ABORT
%token EQ NEQ
%token <str> ID INT_LITERAL FLOAT_LITERAL STRING_LITERAL

%type <ival> type_spec
%type <ptr> program statements statement block declaration assignment input_stmt output_stmt if_stmt while_stmt for_stmt return_stmt break_stmt
%type <ptr> expression for_init for_update assign_nosemi for_cond

%start program

%left EQ NEQ '>' '<'
%left '+' '-'
%left '*' '/'
%right UMINUS

%nonassoc LOWER_THAN_BREACH
%nonassoc BREACH

%%

program
    : statements
      {
          g_program_ast = (StmtList*)$1;
          $$ = $1;
      }
    ;

statements
    : statements statement
      {
          $$ = ast_stmt_list_append((StmtList*)$1, (Stmt*)$2);
      }
    | /* empty */
      {
          $$ = NULL;
      }
    ;

statement
    : declaration { $$ = $1; }
    | assignment  { $$ = $1; }
    | input_stmt  { $$ = $1; }
    | output_stmt { $$ = $1; }
    | if_stmt     { $$ = $1; }
    | while_stmt  { $$ = $1; }
    | for_stmt    { $$ = $1; }
    | return_stmt { $$ = $1; }
    | break_stmt  { $$ = $1; }
    | block       { $$ = $1; }
    ;

block
    : '{' statements '}'
      {
          $$ = ast_make_block((StmtList*)$2);
      }
    ;

declaration
    : type_spec ID ';'
      {
          $$ = ast_make_decl((Type)$1, $2, NULL, 0);
          free($2);
      }
    | type_spec ID '=' expression ';'
      {
          $$ = ast_make_decl((Type)$1, $2, (Expr*)$4, 0);
          free($2);
      }
    ;

assignment
    : ID '=' expression ';'
      {
          $$ = ast_make_assign($1, (Expr*)$3);
          free($1);
      }
    ;

input_stmt
    : SCAN ID ';'
      {
          $$ = ast_make_input($2);
          free($2);
      }
    ;

output_stmt
    : LOG expression ';'
      {
          $$ = ast_make_output((Expr*)$2);
      }
    ;

if_stmt
    : FIREWALL '(' expression ')' statement %prec LOWER_THAN_BREACH
      {
          $$ = ast_make_if((Expr*)$3, (Stmt*)$5, NULL);
      }
    | FIREWALL '(' expression ')' statement BREACH statement
      {
          $$ = ast_make_if((Expr*)$3, (Stmt*)$5, (Stmt*)$7);
      }
    ;

while_stmt
    : MONITOR '(' expression ')' statement
      {
          $$ = ast_make_while((Expr*)$3, (Stmt*)$5);
      }
    ;

for_stmt
    : ITERATE '(' for_init ';' for_cond ';' for_update ')' statement
      {
          $$ = ast_make_for((Stmt*)$3, (Expr*)$5, (Stmt*)$7, (Stmt*)$9);
      }
    ;

for_init
    : assign_nosemi { $$ = $1; }
    | /* empty */   { $$ = NULL; }
    ;

for_update
    : assign_nosemi { $$ = $1; }
    | /* empty */   { $$ = NULL; }
    ;

for_cond
    : expression { $$ = $1; }
    | /* empty */
      {
          $$ = ast_make_int_literal(1);
      }
    ;

assign_nosemi
    : ID '=' expression
      {
          $$ = ast_make_assign($1, (Expr*)$3);
          free($1);
      }
    ;

return_stmt
    : EXITSEC expression ';'
      {
          $$ = ast_make_return((Expr*)$2);
      }
    ;

break_stmt
    : ABORT ';'
      {
          $$ = ast_make_break();
      }
    ;

expression
    : expression '+' expression
      {
          $$ = ast_make_binary(OP_ADD, (Expr*)$1, (Expr*)$3);
      }
    | expression '-' expression
      {
          $$ = ast_make_binary(OP_SUB, (Expr*)$1, (Expr*)$3);
      }
    | expression '*' expression
      {
          $$ = ast_make_binary(OP_MUL, (Expr*)$1, (Expr*)$3);
      }
    | expression '/' expression
      {
          $$ = ast_make_binary(OP_DIV, (Expr*)$1, (Expr*)$3);
      }
    | expression '>' expression
      {
          $$ = ast_make_binary(OP_GT, (Expr*)$1, (Expr*)$3);
      }
    | expression '<' expression
      {
          $$ = ast_make_binary(OP_LT, (Expr*)$1, (Expr*)$3);
      }
    | expression EQ expression
      {
          $$ = ast_make_binary(OP_EQ, (Expr*)$1, (Expr*)$3);
      }
    | expression NEQ expression
      {
          $$ = ast_make_binary(OP_NEQ, (Expr*)$1, (Expr*)$3);
      }
    | '-' expression %prec UMINUS
      {
          $$ = ast_make_unary(OP_NEG, (Expr*)$2);
      }
    | '(' expression ')'
      {
          $$ = $2;
      }
    | ID
      {
          $$ = ast_make_var($1);
          free($1);
      }
    | INT_LITERAL
      {
          $$ = ast_make_int_literal(atoi($1));
          free($1);
      }
    | FLOAT_LITERAL
      {
          $$ = ast_make_float_literal(atof($1));
          free($1);
      }
    | STRING_LITERAL
      {
          char* s = strip_quotes($1);
          $$ = ast_make_string_literal(s);
          free(s);
          free($1);
      }
    ;

type_spec
    : AUTH   { $$ = TYPE_INT; }
    | SECURE { $$ = TYPE_FLOAT; }
    | PACKET { $$ = TYPE_PACKET; }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, s);
}
