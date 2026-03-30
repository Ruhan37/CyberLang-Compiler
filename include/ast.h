#ifndef AST_H
#define AST_H

#include "value.h"

typedef enum {
    EXPR_INT_LITERAL,
    EXPR_FLOAT_LITERAL,
    EXPR_STRING_LITERAL,
    EXPR_VAR,
    EXPR_BINARY,
    EXPR_UNARY
} ExprKind;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_GT,
    OP_LT,
    OP_EQ,
    OP_NEQ,
    OP_NEG
} OpKind;

typedef enum {
    STMT_DECL,
    STMT_ASSIGN,
    STMT_INPUT,
    STMT_OUTPUT,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_RETURN,
    STMT_BREAK,
    STMT_BLOCK
} StmtKind;

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct StmtList StmtList;

struct Expr {
    ExprKind kind;
    Type inferred_type;
    union {
        int int_val;
        double float_val;
        char* str_val;
        char* var_name;
        struct {
            OpKind op;
            Expr* left;
            Expr* right;
        } binary;
        struct {
            OpKind op;
            Expr* expr;
        } unary;
    } u;
};

struct Stmt {
    StmtKind kind;
    union {
        struct {
            Type decl_type;
            char* name;
            Expr* init;
            int is_const;
        } decl;
        struct {
            char* name;
            Expr* expr;
        } assign;
        struct {
            char* name;
        } input;
        struct {
            Expr* expr;
        } output;
        struct {
            Expr* cond;
            Stmt* then_stmt;
            Stmt* else_stmt;
        } if_stmt;
        struct {
            Expr* cond;
            Stmt* body;
        } while_stmt;
        struct {
            Stmt* init;
            Expr* cond;
            Stmt* update;
            Stmt* body;
        } for_stmt;
        struct {
            Expr* expr;
        } ret;
        struct {
            StmtList* statements;
        } block;
    } u;
};

struct StmtList {
    Stmt* stmt;
    StmtList* next;
};

Expr* ast_make_int_literal(int v);
Expr* ast_make_float_literal(double v);
Expr* ast_make_string_literal(const char* s);
Expr* ast_make_var(const char* name);
Expr* ast_make_binary(OpKind op, Expr* left, Expr* right);
Expr* ast_make_unary(OpKind op, Expr* expr);

Stmt* ast_make_decl(Type type, const char* name, Expr* init, int is_const);
Stmt* ast_make_assign(const char* name, Expr* expr);
Stmt* ast_make_input(const char* name);
Stmt* ast_make_output(Expr* expr);
Stmt* ast_make_if(Expr* cond, Stmt* then_stmt, Stmt* else_stmt);
Stmt* ast_make_while(Expr* cond, Stmt* body);
Stmt* ast_make_for(Stmt* init, Expr* cond, Stmt* update, Stmt* body);
Stmt* ast_make_return(Expr* expr);
Stmt* ast_make_break(void);
Stmt* ast_make_block(StmtList* statements);

StmtList* ast_stmt_list_append(StmtList* list, Stmt* stmt);

void ast_free_expr(Expr* expr);
void ast_free_stmt(Stmt* stmt);
void ast_free_stmt_list(StmtList* list);

#endif
