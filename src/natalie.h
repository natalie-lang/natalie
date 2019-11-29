#ifndef __NAT__
#define __NAT__

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

#define UNUSED(x) (void)(x)

#define TRUE 1
#define FALSE 0

typedef struct NatObject NatObject;
typedef struct NatEnv NatEnv;

struct NatEnv {
    struct hashmap data;
    NatEnv *outer;
};

enum NatValueType {
    NAT_VALUE_CLASS,
    NAT_VALUE_MODULE,
    NAT_VALUE_ARRAY,
    NAT_VALUE_STRING,
    NAT_VALUE_NUMBER,
    NAT_VALUE_NIL,
    NAT_VALUE_PROC,
    NAT_VALUE_OTHER
};

#define NAT_FLAG_MAIN_OBJECT 1
#define NAT_FLAG_TOP_CLASS 2

#define nat_is_main_object(obj) (((obj)->flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT)
#define nat_is_top_class(obj) (((obj)->flags & NAT_FLAG_TOP_CLASS) == NAT_FLAG_TOP_CLASS)

struct NatObject {
    enum NatValueType type;
    NatObject *class;
    int flags;

    struct hashmap singleton_methods;
    
    size_t included_modules_count;
    NatObject **included_modules;

    union {
        long long number;
        struct hashmap hashmap;

        // NAT_VALUE_CLASS, NAT_VALUE_MODULE
        struct {
            char *class_name;
            NatObject *superclass;
            struct hashmap methods;
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

        // NAT_VALUE_REGEX
        struct {
            size_t regex_len;
            char *regex;
        };

        // NAT_VALUE_PROC
        struct {
            NatObject* (*fn)(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs);
            char *method_name;
            NatEnv *env;
            size_t argc;
            NatObject **args;
            struct hashmap kwargs;
        };
    };
};

NatEnv *env_find(NatEnv *env, char *key);
NatObject *env_get(NatEnv *env, char *key);
NatObject *env_set(NatEnv *env, char *key, NatObject *val);
NatEnv *build_env(NatEnv *outer);

char *heap_string(char *str);

NatObject *nat_alloc();
NatObject *nat_subclass(NatObject *superclass, char *name);
NatObject *nat_module(NatEnv *env, char *name);
void nat_class_include(NatObject *class, NatObject *module);
NatObject *nat_new(NatObject *class);

NatObject *nat_number(NatEnv *env, long long num);

size_t num_char_len(long long num);
char* long_long_to_string(long long num);

void nat_define_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*));
NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args);
NatObject *nat_lookup_or_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args);
NatObject *nat_call_method_on_class(NatEnv *env, NatObject *class, NatObject *instance_class, char *method_name, NatObject *self, size_t argc, NatObject **args);

#define NAT_STRING_GROW_FACTOR 2

NatObject *nat_string(NatEnv *env, char *str);
void nat_grow_string(NatObject *obj, size_t capacity);
void nat_grow_string_at_least(NatObject *obj, size_t min_capacity);
void nat_string_append(NatObject *str, char *s);
void nat_string_append_char(NatObject *str, char c);

#define NAT_ARRAY_INIT_SIZE 10
#define NAT_ARRAY_GROW_FACTOR 2

NatObject *nat_array(NatEnv *env);
void nat_grow_array(NatObject *obj, size_t capacity);
void nat_grow_array_at_least(NatObject *obj, size_t min_capacity);
void nat_array_push(NatObject *array, NatObject *obj);

#endif
