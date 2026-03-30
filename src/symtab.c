#include "symtab.h"

#include <stdlib.h>
#include <string.h>

typedef struct SymEntry {
    char* name;
    Type type;
    int is_const;
    int has_value;
    Value value;
    struct SymEntry* next;
} SymEntry;

static SymEntry* g_head = NULL;

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

static SymEntry* find_entry(const char* name) {
    SymEntry* cur = g_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void symtab_clear(void) {
    SymEntry* cur = g_head;
    while (cur) {
        SymEntry* next = cur->next;
        free(cur->name);
        value_free(&cur->value);
        free(cur);
        cur = next;
    }
    g_head = NULL;
}

void symtab_reset_runtime(void) {
    SymEntry* cur = g_head;
    while (cur) {
        value_free(&cur->value);
        cur->has_value = 0;
        cur = cur->next;
    }
}

int symtab_declare(const char* name, Type type, int is_const) {
    SymEntry* node;

    if (find_entry(name)) {
        return 0;
    }

    node = (SymEntry*)calloc(1, sizeof(SymEntry));
    if (!node) {
        return 0;
    }

    node->name = clone_str(name);
    node->type = type;
    node->is_const = is_const ? 1 : 0;
    node->has_value = 0;
    node->value = value_invalid();
    node->next = g_head;
    g_head = node;
    return 1;
}

int symtab_exists(const char* name) {
    return find_entry(name) != NULL;
}

int symtab_get_type(const char* name, Type* out_type) {
    SymEntry* e = find_entry(name);
    if (!e || !out_type) {
        return 0;
    }
    *out_type = e->type;
    return 1;
}

int symtab_is_const(const char* name, int* out_is_const) {
    SymEntry* e = find_entry(name);
    if (!e || !out_is_const) {
        return 0;
    }
    *out_is_const = e->is_const;
    return 1;
}

int symtab_set_value(const char* name, const Value* value) {
    SymEntry* e = find_entry(name);
    if (!e || !value || !value->is_set) {
        return 0;
    }

    if (e->is_const && e->has_value) {
        return 0;
    }

    value_free(&e->value);
    e->value = value_copy(value);
    e->has_value = 1;
    return 1;
}

int symtab_get_value(const char* name, Value* out_value) {
    SymEntry* e = find_entry(name);
    if (!e || !e->has_value || !out_value) {
        return 0;
    }

    *out_value = value_copy(&e->value);
    return 1;
}
