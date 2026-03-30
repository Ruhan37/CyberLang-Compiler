#ifndef VALUE_H
#define VALUE_H

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_PACKET,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_INVALID
} Type;

typedef struct {
    Type type;
    int is_set;
    union {
        int i;
        double f;
        long long ll;
        int b;
        char* s;
    } data;
} Value;

typedef enum {
    EXEC_OK,
    EXEC_BREAK,
    EXEC_RETURN
} ExecSignal;

typedef struct {
    ExecSignal signal;
    Value value;
} EvalVal;

const char* type_name(Type type);
int is_numeric_type(Type type);

Value value_int(int v);
Value value_float(double v);
Value value_packet(long long v);
Value value_bool(int v);
Value value_string(const char* s);
Value value_invalid(void);

void value_free(Value* v);
Value value_copy(const Value* v);

int cast_value(const Value* src, Type target, Value* out);

#endif
