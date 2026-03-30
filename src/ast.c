#include "ast.h"

#include <stdlib.h>
#include <string.h>

static char* clone_str(const char* s) {
    size_t n;
    char* out;

    if (!s) {
        return NULL;
    }

    n = strlen(s);
    out = (char*)malloc(n + 1);
    if (!out) {
        return NULL;
    }

    memcpy(out, s, n + 1);
    return out;
}

Expr* ast_make_int_literal(int v) {
    Expr* e = (Expr*)calloc(1, sizeof(Expr));
    e->kind = EXPR_INT_LITERAL;
    e->inferred_type = TYPE_INT;
    e->u.int_val = v;
    return e;
}

Expr* ast_make_float_literal(double v) {
    Expr* e = (Expr*)calloc(1, sizeof(Expr));
    e->kind = EXPR_FLOAT_LITERAL;
    e->inferred_type = TYPE_FLOAT;
    e->u.float_val = v;
    return e;
}

Expr* ast_make_string_literal(const char* s) {
    Expr* e = (Expr*)calloc(1, sizeof(Expr));
    e->kind = EXPR_STRING_LITERAL;
    e->inferred_type = TYPE_STRING;
    e->u.str_val = clone_str(s ? s : "");
    return e;
}

Expr* ast_make_var(const char* name) {
    Expr* e = (Expr*)calloc(1, sizeof(Expr));
    e->kind = EXPR_VAR;
    e->inferred_type = TYPE_INVALID;
    e->u.var_name = clone_str(name);
    return e;
}

Expr* ast_make_binary(OpKind op, Expr* left, Expr* right) {
    Expr* e = (Expr*)calloc(1, sizeof(Expr));
    e->kind = EXPR_BINARY;
    e->inferred_type = TYPE_INVALID;
    e->u.binary.op = op;
    e->u.binary.left = left;
    e->u.binary.right = right;
    return e;
}

Expr* ast_make_unary(OpKind op, Expr* expr) {
    Expr* e = (Expr*)calloc(1, sizeof(Expr));
    e->kind = EXPR_UNARY;
    e->inferred_type = TYPE_INVALID;
    e->u.unary.op = op;
    e->u.unary.expr = expr;
    return e;
}

Stmt* ast_make_decl(Type type, const char* name, Expr* init, int is_const) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_DECL;
    s->u.decl.decl_type = type;
    s->u.decl.name = clone_str(name);
    s->u.decl.init = init;
    s->u.decl.is_const = is_const;
    return s;
}

Stmt* ast_make_assign(const char* name, Expr* expr) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_ASSIGN;
    s->u.assign.name = clone_str(name);
    s->u.assign.expr = expr;
    return s;
}

Stmt* ast_make_input(const char* name) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_INPUT;
    s->u.input.name = clone_str(name);
    return s;
}

Stmt* ast_make_output(Expr* expr) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_OUTPUT;
    s->u.output.expr = expr;
    return s;
}

Stmt* ast_make_if(Expr* cond, Stmt* then_stmt, Stmt* else_stmt) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_IF;
    s->u.if_stmt.cond = cond;
    s->u.if_stmt.then_stmt = then_stmt;
    s->u.if_stmt.else_stmt = else_stmt;
    return s;
}

Stmt* ast_make_while(Expr* cond, Stmt* body) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_WHILE;
    s->u.while_stmt.cond = cond;
    s->u.while_stmt.body = body;
    return s;
}

Stmt* ast_make_for(Stmt* init, Expr* cond, Stmt* update, Stmt* body) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_FOR;
    s->u.for_stmt.init = init;
    s->u.for_stmt.cond = cond;
    s->u.for_stmt.update = update;
    s->u.for_stmt.body = body;
    return s;
}

Stmt* ast_make_return(Expr* expr) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_RETURN;
    s->u.ret.expr = expr;
    return s;
}

Stmt* ast_make_break(void) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_BREAK;
    return s;
}

Stmt* ast_make_block(StmtList* statements) {
    Stmt* s = (Stmt*)calloc(1, sizeof(Stmt));
    s->kind = STMT_BLOCK;
    s->u.block.statements = statements;
    return s;
}

StmtList* ast_stmt_list_append(StmtList* list, Stmt* stmt) {
    StmtList* node;
    StmtList* cur;

    node = (StmtList*)calloc(1, sizeof(StmtList));
    node->stmt = stmt;

    if (!list) {
        return node;
    }

    cur = list;
    while (cur->next) {
        cur = cur->next;
    }
    cur->next = node;
    return list;
}

void ast_free_expr(Expr* expr) {
    if (!expr) {
        return;
    }

    switch (expr->kind) {
        case EXPR_STRING_LITERAL:
            free(expr->u.str_val);
            break;
        case EXPR_VAR:
            free(expr->u.var_name);
            break;
        case EXPR_BINARY:
            ast_free_expr(expr->u.binary.left);
            ast_free_expr(expr->u.binary.right);
            break;
        case EXPR_UNARY:
            ast_free_expr(expr->u.unary.expr);
            break;
        default:
            break;
    }

    free(expr);
}

void ast_free_stmt(Stmt* stmt) {
    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
        case STMT_DECL:
            free(stmt->u.decl.name);
            ast_free_expr(stmt->u.decl.init);
            break;
        case STMT_ASSIGN:
            free(stmt->u.assign.name);
            ast_free_expr(stmt->u.assign.expr);
            break;
        case STMT_INPUT:
            free(stmt->u.input.name);
            break;
        case STMT_OUTPUT:
            ast_free_expr(stmt->u.output.expr);
            break;
        case STMT_IF:
            ast_free_expr(stmt->u.if_stmt.cond);
            ast_free_stmt(stmt->u.if_stmt.then_stmt);
            ast_free_stmt(stmt->u.if_stmt.else_stmt);
            break;
        case STMT_WHILE:
            ast_free_expr(stmt->u.while_stmt.cond);
            ast_free_stmt(stmt->u.while_stmt.body);
            break;
        case STMT_FOR:
            ast_free_stmt(stmt->u.for_stmt.init);
            ast_free_expr(stmt->u.for_stmt.cond);
            ast_free_stmt(stmt->u.for_stmt.update);
            ast_free_stmt(stmt->u.for_stmt.body);
            break;
        case STMT_RETURN:
            ast_free_expr(stmt->u.ret.expr);
            break;
        case STMT_BLOCK:
            ast_free_stmt_list(stmt->u.block.statements);
            break;
        case STMT_BREAK:
            break;
    }

    free(stmt);
}

void ast_free_stmt_list(StmtList* list) {
    StmtList* cur = list;
    while (cur) {
        StmtList* next = cur->next;
        ast_free_stmt(cur->stmt);
        free(cur);
        cur = next;
    }
}
