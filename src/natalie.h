#ifndef __NAT__
#define __NAT__

#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

#define UNUSED(x) (void)(x)

#define NAT_RESCUE(env) setjmp(*(env->jump_buf = malloc(sizeof(jmp_buf))))
#define NAT_RAISE(env, klass, message_format, ...) nat_raise(env, klass, message_format, ##__VA_ARGS__); abort();
#define NAT_ASSERT_ARGC1(expected) if(argc != expected) { NAT_RAISE(env, env_get(env, "ArgumentError"), "wrong number of arguments (given %d, expected %d)", argc, expected); }
#define NAT_ASSERT_ARGC2(expected_low, expected_high) if(argc < expected_low || argc > expected_high) { NAT_RAISE(env, env_get(env, "ArgumentError"), "wrong number of arguments (given %d, expected %d..%d)", argc, expected_low, expected_high); }
#define NAT_ASSERT_ARGC_AT_LEAST(expected) if(argc < expected) { NAT_RAISE(env, env_get(env, "ArgumentError"), "wrong number of arguments (given %d, expected %d+)", argc, expected); }
#define GET_MACRO(_1, _2, NAME, ...) NAME
#define NAT_ASSERT_ARGC(...) GET_MACRO(__VA_ARGS__, NAT_ASSERT_ARGC2, NAT_ASSERT_ARGC1)(__VA_ARGS__)

#define TRUE 1
#define FALSE 0

typedef struct NatObject NatObject;
typedef struct NatEnv NatEnv;
typedef struct NatBlock NatBlock;
typedef struct NatMethod NatMethod;

struct NatEnv {
    struct hashmap data;
    struct hashmap *globals;
    struct hashmap *symbols;
    uint64_t *next_object_id;
    NatEnv *outer;
    int block;
    jmp_buf *jump_buf;
    NatObject *exception;
    NatEnv *caller;
};

struct NatBlock {
    NatObject* (*fn)(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
    NatEnv *env;
    NatObject *self;
};

struct NatMethod {
    NatObject* (*fn)(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
    NatEnv *env; // optional
};

enum NatValueType {
    NAT_VALUE_ARRAY,
    NAT_VALUE_CLASS,
    NAT_VALUE_EXCEPTION,
    NAT_VALUE_FALSE,
    NAT_VALUE_INTEGER,
    NAT_VALUE_MODULE,
    NAT_VALUE_NIL,
    NAT_VALUE_OTHER,
    NAT_VALUE_PROC,
    NAT_VALUE_STRING,
    NAT_VALUE_SYMBOL,
    NAT_VALUE_TRUE
};

#define NAT_FLAG_MAIN_OBJECT 1
#define NAT_FLAG_TOP_CLASS 2

#define nat_is_main_object(obj) (((obj)->flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT)
#define nat_is_top_class(obj) (((obj)->flags & NAT_FLAG_TOP_CLASS) == NAT_FLAG_TOP_CLASS)

struct NatObject {
    enum NatValueType type;
    NatObject *class;
    int flags;

    NatEnv *env;

    int64_t id;

    struct hashmap singleton_methods;
    struct hashmap ivars;
    
    union {
        int64_t integer;
        struct hashmap hashmap;

        // NAT_VALUE_CLASS, NAT_VALUE_MODULE
        struct {
            char *class_name;
            NatObject *superclass;
            struct hashmap methods;
            size_t included_modules_count;
            NatObject **included_modules;
        };

        // NAT_VALUE_ARRAY
        struct {
            size_t ary_len;
            size_t ary_cap;
            NatObject **ary;
        };

        // NAT_VALUE_STRING
        struct {
            size_t str_len;
            size_t str_cap;
            char *str;
        };

        // NAT_VALUE_SYMBOL
        char *symbol;

        // NAT_VALUE_REGEX
        struct {
            size_t regex_len;
            char *regex;
        };

        // NAT_VALUE_EXCEPTION
        char *message;

        // NAT_VALUE_PROC
        NatBlock *block;
    };
};

int is_constant_name(char *name);
int is_special_name(char *name);

NatObject *env_get(NatEnv *env, char *key);
NatObject *env_set(NatEnv *env, char *key, NatObject *val);
NatEnv *build_env(NatEnv *outer);
NatEnv *build_block_env(NatEnv *outer, NatEnv *calling_env);

NatObject* nat_raise(NatEnv *env, NatObject *klass, char *message_format, ...);
NatObject* nat_raise_exception(NatEnv *env, NatObject *exception);
int nat_rescue(NatEnv *env);

NatObject *ivar_get(NatEnv *env, NatObject *obj, char *name);
void ivar_set(NatEnv *env, NatObject *obj, char *name, NatObject *val);

NatObject *global_get(NatEnv *env, char *name);
void global_set(NatEnv *env, char *name, NatObject *val);

int nat_truthy(NatObject *obj);

char *heap_string(char *str);

NatObject *nat_alloc(NatEnv *env);
NatObject *nat_subclass(NatEnv *env, NatObject *superclass, char *name);
NatObject *nat_module(NatEnv *env, char *name);
void nat_class_include(NatObject *class, NatObject *module);
NatObject *nat_new(NatEnv *env, NatObject *class, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

NatObject *nat_integer(NatEnv *env, int64_t integer);

char *nat_object_pointer_id(NatObject *obj);
uint64_t nat_next_object_id(NatEnv *env);

char* int_to_string(int64_t num);

void nat_define_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *block));
void nat_define_method_with_block(NatObject *obj, char *name, NatBlock *block);
void nat_define_singleton_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *block));

NatObject *nat_class_ancestors(NatEnv *env, NatObject *klass);
int nat_is_a(NatEnv *env, NatObject *obj, NatObject *klass_or_module);

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args, NatBlock *block);
NatObject *nat_lookup_or_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args, NatBlock *block);
NatObject *nat_lookup(NatEnv *env, char *name);
NatObject *nat_call_method_on_class(NatEnv *env, NatObject *class, NatObject *instance_class, char *method_name, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
int nat_respond_to(NatObject *obj, char *name);

NatBlock *nat_block(NatEnv *env, NatObject *self, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock*));
NatObject *nat_run_block(NatEnv *env, NatBlock *the_block, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *nat_proc(NatEnv *env, NatBlock *block);

#define NAT_STRING_GROW_FACTOR 2

NatObject *nat_string(NatEnv *env, char *str);
void nat_grow_string(NatObject *obj, size_t capacity);
void nat_grow_string_at_least(NatObject *obj, size_t min_capacity);
void nat_string_append(NatObject *str, char *s);
void nat_string_append_char(NatObject *str, char c);
NatObject* nat_sprintf(NatEnv *env, char *format, ...);
NatObject* nat_vsprintf(NatEnv *env, char *format, va_list args);

NatObject *nat_symbol(NatEnv *env, char *name);

NatObject *nat_exception(NatEnv *env, NatObject *klass, char *message);

#define NAT_ARRAY_INIT_SIZE 10
#define NAT_ARRAY_GROW_FACTOR 2

NatObject *nat_array(NatEnv *env);
void nat_grow_array(NatObject *obj, size_t capacity);
void nat_grow_array_at_least(NatObject *obj, size_t min_capacity);
void nat_array_push(NatObject *array, NatObject *obj);
void nat_array_push_splat(NatEnv *env, NatObject *array, NatObject *obj);

#endif
