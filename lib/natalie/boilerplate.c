#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum NatTypeType {
    NAT_NIL_TYPE,
    NAT_TRUE_TYPE,
    NAT_FALSE_TYPE,
    NAT_KEYWORD_TYPE,
    NAT_NUMBER_TYPE,
    NAT_SYMBOL_TYPE,
    NAT_ARRAY_TYPE,
    NAT_HASHMAP_TYPE,
    NAT_STRING_TYPE,
    NAT_REGEX_TYPE,
    NAT_LAMBDA_TYPE,
    NAT_CONTINUATION_TYPE,
    NAT_ATOM_TYPE,
    NAT_BLANK_LINE_TYPE,
    NAT_ERROR_TYPE
};

typedef struct NatType NatType;

struct NatType {
enum NatTypeType type;

union {
    long long number;
    char *symbol;
    char *keyword;
    //struct hashmap hashmap;
    NatType *atom_val;
    NatType *error_val;

    // NAT_ARRAY_TYPE
    struct {
    size_t ary_len;
    size_t ary_cap;
    NatType **ary;
    };

    // NAT_STRING_TYPE
    struct {
    size_t str_len;
    size_t str_cap;
    char *str;
    };

    // NAT_REGEX_TYPE
    struct {
    size_t regex_len;
    char *regex;
    };

    // NAT_LAMBDA_TYPE, NAT_CONTINUATION_TYPE
    //struct {
    //  NatType* (*fn)(NatEnv *env, size_t argc, NatType **args);
    //  char *function_name;
    //  NatEnv *env;
    //  size_t argc;
    //  NatType **args;
    //};
};
};

NatType* nat_alloc() {
    NatType *val = malloc(sizeof(NatType));
    return val;
}

NatType* nat_number(long long num) {
    NatType *val = nat_alloc();
    val->type = NAT_NUMBER_TYPE;
    val->number = num;
    return val;
}

NatType* nat_string(char *str) {
    NatType *val = nat_alloc();
    val->type = NAT_STRING_TYPE;
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    memcpy(copy, str, len + 1);
    val->str = copy;
    val->str_len = len;
    val->str_cap = len;
    return val;
}

int main() {
    __MAIN__
    return 0;
}