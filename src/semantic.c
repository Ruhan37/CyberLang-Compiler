#include "semantic.h"

#include "symtab.h"

#include <stdarg.h>
#include <stdio.h>

static int g_sem_errors = 0;

static void sem_error(const char* fmt, ...) {
    va_list ap;

    ++g_sem_errors;
    fprintf(stderr, "Semantic error: ");

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");
}

static int can_assign(Type target, Type src) {
    if (target == src) {
        return 1;
    }
    if (target == TYPE_FLOAT && (src == TYPE_INT || src == TYPE_PACKET)) {
        return 1;
    }
    if (target == TYPE_PACKET && src == TYPE_INT) {
        return 1;
    }
    return 0;
}

static Type promote_numeric(Type a, Type b) {
    if (a == TYPE_FLOAT || b == TYPE_FLOAT) {
        return TYPE_FLOAT;
    }
    if (a == TYPE_PACKET || b == TYPE_PACKET) {
        return TYPE_PACKET;
    }
    return TYPE_INT;
}

static Type check_expr(Expr* expr);
static void check_stmt(Stmt* stmt);
static void check_stmt_list(StmtList* list);

static Type check_expr(Expr* expr) {
    Type left;
    Type right;

    if (!expr) {
        sem_error("Null expression");
        return TYPE_INVALID;
    }

    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            expr->inferred_type = TYPE_INT;
            return TYPE_INT;

        case EXPR_FLOAT_LITERAL:
            expr->inferred_type = TYPE_FLOAT;
            return TYPE_FLOAT;

        case EXPR_STRING_LITERAL:
            expr->inferred_type = TYPE_STRING;
            return TYPE_STRING;

        case EXPR_VAR:
            if (!symtab_get_type(expr->u.var_name, &expr->inferred_type)) {
                sem_error("Variable '%s' used before declaration", expr->u.var_name);
                expr->inferred_type = TYPE_INVALID;
            }
            return expr->inferred_type;

        case EXPR_UNARY:
            left = check_expr(expr->u.unary.expr);
            if (expr->u.unary.op == OP_NEG) {
                if (!is_numeric_type(left)) {
                    sem_error("Unary '-' expects numeric operand");
                    expr->inferred_type = TYPE_INVALID;
                } else {
                    expr->inferred_type = left;
                }
                return expr->inferred_type;
            }
            sem_error("Unknown unary operation");
            expr->inferred_type = TYPE_INVALID;
            return TYPE_INVALID;

        case EXPR_BINARY:
            left = check_expr(expr->u.binary.left);
            right = check_expr(expr->u.binary.right);

            switch (expr->u.binary.op) {
                case OP_ADD:
                case OP_SUB:
                case OP_MUL:
                case OP_DIV:
                    if (!is_numeric_type(left) || !is_numeric_type(right)) {
                        sem_error("Arithmetic operators require numeric operands");
                        expr->inferred_type = TYPE_INVALID;
                    } else {
                        expr->inferred_type = promote_numeric(left, right);
                    }
                    return expr->inferred_type;

                case OP_GT:
                case OP_LT:
                case OP_EQ:
                case OP_NEQ:
                    if (!is_numeric_type(left) || !is_numeric_type(right)) {
                        sem_error("Relational operators require numeric operands");
                        expr->inferred_type = TYPE_INVALID;
                    } else {
                        expr->inferred_type = TYPE_BOOL;
                    }
                    return expr->inferred_type;

                default:
                    sem_error("Unknown binary operation");
                    expr->inferred_type = TYPE_INVALID;
                    return TYPE_INVALID;
            }

        default:
            sem_error("Unknown expression kind");
            return TYPE_INVALID;
    }
}

static int cond_type_ok(Type t) {
    return t == TYPE_BOOL || is_numeric_type(t);
}

static void check_stmt(Stmt* stmt) {
    Type lhs_type;
    Type rhs_type;
    int is_const;

    if (!stmt) {
        return;
    }

    switch (stmt->kind) {
        case STMT_DECL:
            if (!symtab_declare(stmt->u.decl.name, stmt->u.decl.decl_type, stmt->u.decl.is_const)) {
                sem_error("Redeclaration of variable '%s'", stmt->u.decl.name);
                return;
            }

            if (stmt->u.decl.init) {
                rhs_type = check_expr(stmt->u.decl.init);
                if (!can_assign(stmt->u.decl.decl_type, rhs_type)) {
                    sem_error("Type mismatch in declaration '%s' (%s <- %s)", stmt->u.decl.name,
                              type_name(stmt->u.decl.decl_type), type_name(rhs_type));
                }
            }
            break;

        case STMT_ASSIGN:
            if (!symtab_get_type(stmt->u.assign.name, &lhs_type)) {
                sem_error("Variable '%s' used before declaration", stmt->u.assign.name);
                return;
            }

            if (symtab_is_const(stmt->u.assign.name, &is_const) && is_const) {
                sem_error("Cannot assign to constant '%s'", stmt->u.assign.name);
            }

            rhs_type = check_expr(stmt->u.assign.expr);
            if (!can_assign(lhs_type, rhs_type)) {
                sem_error("Type mismatch in assignment '%s' (%s <- %s)", stmt->u.assign.name,
                          type_name(lhs_type), type_name(rhs_type));
            }
            break;

        case STMT_INPUT:
            if (!symtab_get_type(stmt->u.input.name, &lhs_type)) {
                sem_error("Variable '%s' used before declaration", stmt->u.input.name);
                return;
            }
            if (symtab_is_const(stmt->u.input.name, &is_const) && is_const) {
                sem_error("Cannot read into constant '%s'", stmt->u.input.name);
            }
            break;

        case STMT_OUTPUT:
            (void)check_expr(stmt->u.output.expr);
            break;

        case STMT_IF:
            rhs_type = check_expr(stmt->u.if_stmt.cond);
            if (!cond_type_ok(rhs_type)) {
                sem_error("If condition must be bool or numeric");
            }
            check_stmt(stmt->u.if_stmt.then_stmt);
            check_stmt(stmt->u.if_stmt.else_stmt);
            break;

        case STMT_WHILE:
            rhs_type = check_expr(stmt->u.while_stmt.cond);
            if (!cond_type_ok(rhs_type)) {
                sem_error("While condition must be bool or numeric");
            }
            check_stmt(stmt->u.while_stmt.body);
            break;

        case STMT_FOR:
            check_stmt(stmt->u.for_stmt.init);
            rhs_type = check_expr(stmt->u.for_stmt.cond);
            if (!cond_type_ok(rhs_type)) {
                sem_error("For condition must be bool or numeric");
            }
            check_stmt(stmt->u.for_stmt.update);
            check_stmt(stmt->u.for_stmt.body);
            break;

        case STMT_RETURN:
            (void)check_expr(stmt->u.ret.expr);
            break;

        case STMT_BREAK:
            break;

        case STMT_BLOCK:
            check_stmt_list(stmt->u.block.statements);
            break;
    }
}

static void check_stmt_list(StmtList* list) {
    StmtList* cur = list;
    while (cur) {
        check_stmt(cur->stmt);
        cur = cur->next;
    }
}

int semantic_check(StmtList* program) {
    g_sem_errors = 0;
    symtab_clear();

    check_stmt_list(program);
    return g_sem_errors;
}
