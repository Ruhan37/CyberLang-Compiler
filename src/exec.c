#include "exec.h"

#include "symtab.h"

#include <stdio.h>
#include <string.h>

static Value eval_expr(Expr* expr);
static EvalVal exec_stmt(Stmt* stmt);
static EvalVal exec_stmt_list(StmtList* list);

static int value_truthy(const Value* v) {
    if (!v || !v->is_set) {
        return 0;
    }

    switch (v->type) {
        case TYPE_BOOL:
            return v->data.b != 0;
        case TYPE_INT:
            return v->data.i != 0;
        case TYPE_FLOAT:
            return v->data.f != 0.0;
        case TYPE_PACKET:
            return v->data.ll != 0;
        case TYPE_STRING:
            return v->data.s && v->data.s[0] != '\0';
        default:
            return 0;
    }
}

static double value_as_double(const Value* v) {
    switch (v->type) {
        case TYPE_INT:
            return (double)v->data.i;
        case TYPE_FLOAT:
            return v->data.f;
        case TYPE_PACKET:
            return (double)v->data.ll;
        case TYPE_BOOL:
            return (double)v->data.b;
        default:
            return 0.0;
    }
}

static Value numeric_bin(OpKind op, const Value* a, const Value* b) {
    Type promoted;

    if (!is_numeric_type(a->type) || !is_numeric_type(b->type)) {
        fprintf(stderr, "Runtime error: arithmetic operands must be numeric\n");
        return value_invalid();
    }

    if (op == OP_GT || op == OP_LT || op == OP_EQ || op == OP_NEQ) {
        double av = value_as_double(a);
        double bv = value_as_double(b);
        switch (op) {
            case OP_GT:
                return value_bool(av > bv);
            case OP_LT:
                return value_bool(av < bv);
            case OP_EQ:
                return value_bool(av == bv);
            case OP_NEQ:
                return value_bool(av != bv);
            default:
                break;
        }
    }

    promoted = TYPE_INT;
    if (a->type == TYPE_FLOAT || b->type == TYPE_FLOAT) {
        promoted = TYPE_FLOAT;
    } else if (a->type == TYPE_PACKET || b->type == TYPE_PACKET) {
        promoted = TYPE_PACKET;
    }

    if (promoted == TYPE_FLOAT) {
        double av = value_as_double(a);
        double bv = value_as_double(b);
        if (op == OP_ADD) return value_float(av + bv);
        if (op == OP_SUB) return value_float(av - bv);
        if (op == OP_MUL) return value_float(av * bv);
        if (op == OP_DIV) return value_float(av / bv);
    }

    if (promoted == TYPE_PACKET) {
        long long av = (long long)value_as_double(a);
        long long bv = (long long)value_as_double(b);
        if (op == OP_ADD) return value_packet(av + bv);
        if (op == OP_SUB) return value_packet(av - bv);
        if (op == OP_MUL) return value_packet(av * bv);
        if (op == OP_DIV) return value_packet(av / bv);
    }

    {
        int av = (int)value_as_double(a);
        int bv = (int)value_as_double(b);
        if (op == OP_ADD) return value_int(av + bv);
        if (op == OP_SUB) return value_int(av - bv);
        if (op == OP_MUL) return value_int(av * bv);
        if (op == OP_DIV) return value_int(av / bv);
    }

    return value_invalid();
}

static Value eval_expr(Expr* expr) {
    Value left;
    Value right;
    Value out;

    if (!expr) {
        return value_invalid();
    }

    switch (expr->kind) {
        case EXPR_INT_LITERAL:
            return value_int(expr->u.int_val);
        case EXPR_FLOAT_LITERAL:
            return value_float(expr->u.float_val);
        case EXPR_STRING_LITERAL:
            return value_string(expr->u.str_val ? expr->u.str_val : "");
        case EXPR_VAR:
            if (!symtab_get_value(expr->u.var_name, &out)) {
                fprintf(stderr, "Runtime error: variable '%s' is uninitialized\n", expr->u.var_name);
                return value_invalid();
            }
            return out;
        case EXPR_UNARY:
            left = eval_expr(expr->u.unary.expr);
            if (!left.is_set) {
                return left;
            }
            if (expr->u.unary.op == OP_NEG) {
                if (left.type == TYPE_INT) {
                    out = value_int(-left.data.i);
                } else if (left.type == TYPE_FLOAT) {
                    out = value_float(-left.data.f);
                } else if (left.type == TYPE_PACKET) {
                    out = value_packet(-left.data.ll);
                } else {
                    fprintf(stderr, "Runtime error: unary '-' requires numeric operand\n");
                    out = value_invalid();
                }
                value_free(&left);
                return out;
            }
            value_free(&left);
            return value_invalid();
        case EXPR_BINARY:
            left = eval_expr(expr->u.binary.left);
            right = eval_expr(expr->u.binary.right);
            if (!left.is_set || !right.is_set) {
                value_free(&left);
                value_free(&right);
                return value_invalid();
            }
            out = numeric_bin(expr->u.binary.op, &left, &right);
            value_free(&left);
            value_free(&right);
            return out;
    }

    return value_invalid();
}

static EvalVal make_signal(ExecSignal sig, Value v) {
    EvalVal out;
    out.signal = sig;
    out.value = v;
    return out;
}

static EvalVal exec_stmt(Stmt* stmt) {
    Value v;
    Value casted;
    Type target;
    EvalVal body_res;
    int ok;

    if (!stmt) {
        return make_signal(EXEC_OK, value_invalid());
    }

    switch (stmt->kind) {
        case STMT_DECL:
            if (stmt->u.decl.init) {
                v = eval_expr(stmt->u.decl.init);
                if (!v.is_set) {
                    return make_signal(EXEC_OK, value_invalid());
                }
                if (!cast_value(&v, stmt->u.decl.decl_type, &casted)) {
                    fprintf(stderr, "Runtime error: cannot initialize '%s' with %s\n",
                            stmt->u.decl.name, type_name(v.type));
                    value_free(&v);
                    return make_signal(EXEC_OK, value_invalid());
                }
                symtab_set_value(stmt->u.decl.name, &casted);
                value_free(&v);
                value_free(&casted);
            }
            return make_signal(EXEC_OK, value_invalid());

        case STMT_ASSIGN:
            v = eval_expr(stmt->u.assign.expr);
            if (!v.is_set) {
                return make_signal(EXEC_OK, value_invalid());
            }
            if (!symtab_get_type(stmt->u.assign.name, &target)) {
                fprintf(stderr, "Runtime error: unknown variable '%s'\n", stmt->u.assign.name);
                value_free(&v);
                return make_signal(EXEC_OK, value_invalid());
            }
            if (!cast_value(&v, target, &casted)) {
                fprintf(stderr, "Runtime error: cannot assign %s to %s\n", type_name(v.type), type_name(target));
                value_free(&v);
                return make_signal(EXEC_OK, value_invalid());
            }
            if (!symtab_set_value(stmt->u.assign.name, &casted)) {
                fprintf(stderr, "Runtime error: failed to assign '%s' (constant?)\n", stmt->u.assign.name);
            }
            value_free(&v);
            value_free(&casted);
            return make_signal(EXEC_OK, value_invalid());

        case STMT_INPUT:
            if (!symtab_get_type(stmt->u.input.name, &target)) {
                fprintf(stderr, "Runtime error: unknown variable '%s'\n", stmt->u.input.name);
                return make_signal(EXEC_OK, value_invalid());
            }
            if (target == TYPE_INT) {
                int x;
                if (scanf("%d", &x) == 1) {
                    v = value_int(x);
                    symtab_set_value(stmt->u.input.name, &v);
                    value_free(&v);
                }
            } else if (target == TYPE_FLOAT) {
                double x;
                if (scanf("%lf", &x) == 1) {
                    v = value_float(x);
                    symtab_set_value(stmt->u.input.name, &v);
                    value_free(&v);
                }
            } else if (target == TYPE_PACKET) {
                long long x;
                if (scanf("%lld", &x) == 1) {
                    v = value_packet(x);
                    symtab_set_value(stmt->u.input.name, &v);
                    value_free(&v);
                }
            }
            return make_signal(EXEC_OK, value_invalid());

        case STMT_OUTPUT:
            v = eval_expr(stmt->u.output.expr);
            if (!v.is_set) {
                return make_signal(EXEC_OK, value_invalid());
            }
            if (v.type == TYPE_INT) {
                printf("%d\n", v.data.i);
            } else if (v.type == TYPE_FLOAT) {
                printf("%g\n", v.data.f);
            } else if (v.type == TYPE_PACKET) {
                printf("%lld\n", v.data.ll);
            } else if (v.type == TYPE_BOOL) {
                printf("%s\n", v.data.b ? "true" : "false");
            } else if (v.type == TYPE_STRING) {
                printf("%s\n", v.data.s ? v.data.s : "");
            }
            value_free(&v);
            return make_signal(EXEC_OK, value_invalid());

        case STMT_IF:
            v = eval_expr(stmt->u.if_stmt.cond);
            ok = value_truthy(&v);
            value_free(&v);
            if (ok) {
                return exec_stmt(stmt->u.if_stmt.then_stmt);
            }
            return exec_stmt(stmt->u.if_stmt.else_stmt);

        case STMT_WHILE:
            while (1) {
                v = eval_expr(stmt->u.while_stmt.cond);
                ok = value_truthy(&v);
                value_free(&v);
                if (!ok) {
                    break;
                }
                body_res = exec_stmt(stmt->u.while_stmt.body);
                if (body_res.signal == EXEC_BREAK) {
                    return make_signal(EXEC_OK, value_invalid());
                }
                if (body_res.signal == EXEC_RETURN) {
                    return body_res;
                }
            }
            return make_signal(EXEC_OK, value_invalid());

        case STMT_FOR:
            if (stmt->u.for_stmt.init) {
                body_res = exec_stmt(stmt->u.for_stmt.init);
                if (body_res.signal == EXEC_RETURN) {
                    return body_res;
                }
            }
            while (1) {
                v = eval_expr(stmt->u.for_stmt.cond);
                ok = value_truthy(&v);
                value_free(&v);
                if (!ok) {
                    break;
                }
                body_res = exec_stmt(stmt->u.for_stmt.body);
                if (body_res.signal == EXEC_RETURN) {
                    return body_res;
                }
                if (body_res.signal == EXEC_BREAK) {
                    return make_signal(EXEC_OK, value_invalid());
                }
                if (stmt->u.for_stmt.update) {
                    body_res = exec_stmt(stmt->u.for_stmt.update);
                    if (body_res.signal == EXEC_RETURN) {
                        return body_res;
                    }
                }
            }
            return make_signal(EXEC_OK, value_invalid());

        case STMT_RETURN:
            v = eval_expr(stmt->u.ret.expr);
            return make_signal(EXEC_RETURN, v);

        case STMT_BREAK:
            return make_signal(EXEC_BREAK, value_invalid());

        case STMT_BLOCK:
            return exec_stmt_list(stmt->u.block.statements);
    }

    return make_signal(EXEC_OK, value_invalid());
}

static EvalVal exec_stmt_list(StmtList* list) {
    StmtList* cur = list;
    while (cur) {
        EvalVal r = exec_stmt(cur->stmt);
        if (r.signal != EXEC_OK) {
            return r;
        }
        cur = cur->next;
    }
    return make_signal(EXEC_OK, value_invalid());
}

EvalVal exec_program(StmtList* program) {
    symtab_reset_runtime();
    return exec_stmt_list(program);
}
