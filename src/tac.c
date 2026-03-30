#include "tac.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static int temp_counter = 0;
static int label_counter = 0;

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

static char* new_temp(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "t%d", ++temp_counter);
    return clone_str(buf);
}

static char* new_label(void) {
    char buf[32];
    snprintf(buf, sizeof(buf), "L%d", ++label_counter);
    return clone_str(buf);
}

static void emit(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("TAC: ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

static char* emit_expr(Expr* expr);
static void emit_stmt(Stmt* stmt);
static void emit_stmt_list(StmtList* list);

static char* emit_literal_int(int v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", v);
    return clone_str(buf);
}

static char* emit_literal_float(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", v);
    return clone_str(buf);
}

static char* emit_expr(Expr* expr) {
    char* left;
    char* right;
    char* tmp;

    if (!expr) {
        return clone_str("0");
    }

    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            return emit_literal_int(expr->u.int_val);

        case EXPR_FLOAT_LITERAL:
            return emit_literal_float(expr->u.float_val);

        case EXPR_STRING_LITERAL: {
            size_t n = strlen(expr->u.str_val ? expr->u.str_val : "");
            char* out = (char*)malloc(n + 3);
            out[0] = '"';
            memcpy(out + 1, expr->u.str_val ? expr->u.str_val : "", n);
            out[n + 1] = '"';
            out[n + 2] = '\0';
            return out;
        }

        case EXPR_VAR:
            return clone_str(expr->u.var_name);

        case EXPR_UNARY:
            left = emit_expr(expr->u.unary.expr);
            tmp = new_temp();
            emit("%s = -%s", tmp, left);
            free(left);
            return tmp;

        case EXPR_BINARY:
            left = emit_expr(expr->u.binary.left);
            right = emit_expr(expr->u.binary.right);
            tmp = new_temp();
            switch (expr->u.binary.op) {
                case OP_ADD:
                    emit("%s = %s + %s", tmp, left, right);
                    break;
                case OP_SUB:
                    emit("%s = %s - %s", tmp, left, right);
                    break;
                case OP_MUL:
                    emit("%s = %s * %s", tmp, left, right);
                    break;
                case OP_DIV:
                    emit("%s = %s / %s", tmp, left, right);
                    break;
                case OP_GT:
                    emit("%s = %s > %s", tmp, left, right);
                    break;
                case OP_LT:
                    emit("%s = %s < %s", tmp, left, right);
                    break;
                case OP_EQ:
                    emit("%s = %s == %s", tmp, left, right);
                    break;
                case OP_NEQ:
                    emit("%s = %s != %s", tmp, left, right);
                    break;
                default:
                    break;
            }
            free(left);
            free(right);
            return tmp;
    }

    return clone_str("0");
}

static void emit_stmt(Stmt* stmt) {
    char* cond;
    char* start;
    char* els;
    char* done;
    char* val;

    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
        case STMT_DECL:
            if (stmt->u.decl.init) {
                val = emit_expr(stmt->u.decl.init);
                emit("%s = %s", stmt->u.decl.name, val);
                free(val);
            }
            break;

        case STMT_ASSIGN:
            val = emit_expr(stmt->u.assign.expr);
            emit("%s = %s", stmt->u.assign.name, val);
            free(val);
            break;

        case STMT_INPUT:
            emit("read %s", stmt->u.input.name);
            break;

        case STMT_OUTPUT:
            val = emit_expr(stmt->u.output.expr);
            emit("write %s", val);
            free(val);
            break;

        case STMT_IF:
            cond = emit_expr(stmt->u.if_stmt.cond);
            els = new_label();
            done = new_label();
            emit("ifFalse %s goto %s", cond, els);
            emit_stmt(stmt->u.if_stmt.then_stmt);
            emit("goto %s", done);
            emit("%s:", els);
            if (stmt->u.if_stmt.else_stmt) {
                emit_stmt(stmt->u.if_stmt.else_stmt);
            }
            emit("%s:", done);
            free(cond);
            free(els);
            free(done);
            break;

        case STMT_WHILE:
            start = new_label();
            done = new_label();
            emit("%s:", start);
            cond = emit_expr(stmt->u.while_stmt.cond);
            emit("ifFalse %s goto %s", cond, done);
            emit_stmt(stmt->u.while_stmt.body);
            emit("goto %s", start);
            emit("%s:", done);
            free(cond);
            free(start);
            free(done);
            break;

        case STMT_FOR:
            start = new_label();
            done = new_label();
            emit_stmt(stmt->u.for_stmt.init);
            emit("%s:", start);
            cond = emit_expr(stmt->u.for_stmt.cond);
            emit("ifFalse %s goto %s", cond, done);
            emit_stmt(stmt->u.for_stmt.body);
            emit_stmt(stmt->u.for_stmt.update);
            emit("goto %s", start);
            emit("%s:", done);
            free(cond);
            free(start);
            free(done);
            break;

        case STMT_RETURN:
            val = emit_expr(stmt->u.ret.expr);
            emit("return %s", val);
            free(val);
            break;

        case STMT_BREAK:
            emit("break");
            break;

        case STMT_BLOCK:
            emit_stmt_list(stmt->u.block.statements);
            break;
    }
}

static void emit_stmt_list(StmtList* list) {
    StmtList* cur = list;
    while (cur) {
        emit_stmt(cur->stmt);
        cur = cur->next;
    }
}

void tac_generate(StmtList* program) {
    temp_counter = 0;
    label_counter = 0;
    emit_stmt_list(program);
}
