#include "functab.h"

#include <stdlib.h>
#include <string.h>

typedef struct FuncEntry {
    char* name;
    struct FuncEntry* next;
} FuncEntry;

static FuncEntry* g_funcs = NULL;

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

void functab_clear(void) {
    FuncEntry* cur = g_funcs;
    while (cur) {
        FuncEntry* next = cur->next;
        free(cur->name);
        free(cur);
        cur = next;
    }
    g_funcs = NULL;
}

int functab_exists(const char* name) {
    FuncEntry* cur = g_funcs;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

int functab_register(const char* name) {
    FuncEntry* node;

    if (functab_exists(name)) {
        return 0;
    }

    node = (FuncEntry*)calloc(1, sizeof(FuncEntry));
    if (!node) {
        return 0;
    }

    node->name = clone_str(name);
    node->next = g_funcs;
    g_funcs = node;
    return 1;
}
