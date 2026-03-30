#include "value.h"

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

const char* type_name(Type type) {
    switch (type) {
        case TYPE_INT:
            return "int";
        case TYPE_FLOAT:
            return "float";
        case TYPE_PACKET:
            return "long long";
        case TYPE_BOOL:
            return "bool";
        case TYPE_STRING:
            return "string";
        default:
            return "invalid";
    }
}

int is_numeric_type(Type type) {
    return type == TYPE_INT || type == TYPE_FLOAT || type == TYPE_PACKET;
}

Value value_int(int v) {
    Value out;
    out.type = TYPE_INT;
    out.is_set = 1;
    out.data.i = v;
    return out;
}

Value value_float(double v) {
    Value out;
    out.type = TYPE_FLOAT;
    out.is_set = 1;
    out.data.f = v;
    return out;
}

Value value_packet(long long v) {
    Value out;
    out.type = TYPE_PACKET;
    out.is_set = 1;
    out.data.ll = v;
    return out;
}

Value value_bool(int v) {
    Value out;
    out.type = TYPE_BOOL;
    out.is_set = 1;
    out.data.b = v ? 1 : 0;
    return out;
}

Value value_string(const char* s) {
    Value out;
    out.type = TYPE_STRING;
    out.is_set = 1;
    out.data.s = clone_str(s ? s : "");
    return out;
}

Value value_invalid(void) {
    Value out;
    out.type = TYPE_INVALID;
    out.is_set = 0;
    out.data.i = 0;
    return out;
}

void value_free(Value* v) {
    if (!v) {
        return;
    }

    if (v->type == TYPE_STRING && v->is_set && v->data.s) {
        free(v->data.s);
        v->data.s = NULL;
    }

    v->type = TYPE_INVALID;
    v->is_set = 0;
}

Value value_copy(const Value* v) {
    Value out;

    if (!v) {
        return value_invalid();
    }

    out = *v;
    if (v->type == TYPE_STRING && v->is_set) {
        out.data.s = clone_str(v->data.s ? v->data.s : "");
    }

    return out;
}

int cast_value(const Value* src, Type target, Value* out) {
    if (!src || !out || !src->is_set) {
        return 0;
    }

    if (src->type == target) {
        *out = value_copy(src);
        return 1;
    }

    if (target == TYPE_STRING || src->type == TYPE_STRING) {
        return 0;
    }

    if (target == TYPE_BOOL && is_numeric_type(src->type)) {
        if (src->type == TYPE_INT) {
            *out = value_bool(src->data.i != 0);
            return 1;
        }
        if (src->type == TYPE_FLOAT) {
            *out = value_bool(src->data.f != 0.0);
            return 1;
        }
        if (src->type == TYPE_PACKET) {
            *out = value_bool(src->data.ll != 0);
            return 1;
        }
    }

    if (!is_numeric_type(src->type) || !is_numeric_type(target)) {
        return 0;
    }

    if (target == TYPE_FLOAT) {
        if (src->type == TYPE_INT) {
            *out = value_float((double)src->data.i);
            return 1;
        }
        if (src->type == TYPE_PACKET) {
            *out = value_float((double)src->data.ll);
            return 1;
        }
    }

    if (target == TYPE_PACKET) {
        if (src->type == TYPE_INT) {
            *out = value_packet((long long)src->data.i);
            return 1;
        }
        if (src->type == TYPE_FLOAT) {
            *out = value_packet((long long)src->data.f);
            return 1;
        }
    }

    if (target == TYPE_INT) {
        if (src->type == TYPE_FLOAT) {
            *out = value_int((int)src->data.f);
            return 1;
        }
        if (src->type == TYPE_PACKET) {
            *out = value_int((int)src->data.ll);
            return 1;
        }
    }

    return 0;
}
