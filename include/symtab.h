#ifndef SYMTAB_H
#define SYMTAB_H

#include "value.h"

void symtab_clear(void);
void symtab_reset_runtime(void);

int symtab_declare(const char* name, Type type, int is_const);
int symtab_exists(const char* name);
int symtab_get_type(const char* name, Type* out_type);
int symtab_is_const(const char* name, int* out_is_const);

int symtab_set_value(const char* name, const Value* value);
int symtab_get_value(const char* name, Value* out_value);

#endif
