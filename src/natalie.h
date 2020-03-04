#ifndef __NAT__
#define __NAT__

#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "onigmo.h"
#include "hashmap.h"

#define xDEBUG_METHOD_RESOLUTION

#define NAT_IS_TAGGED_INT(obj) ((int64_t)obj & 1)
#define NAT_TYPE(obj) (NAT_IS_TAGGED_INT(obj) ? NAT_VALUE_INTEGER : obj->type)
#define NAT_OBJ_CLASS(obj) (NAT_IS_TAGGED_INT(obj) ? NAT_INTEGER : obj->klass)
#define NAT_RESCUE(env) ((env->rescue = 1) && setjmp(env->jump_buf))
#define NAT_RAISE(env, klass, message_format, ...) nat_raise(env, klass, message_format, ##__VA_ARGS__); abort();
#define NAT_ASSERT_ARGC1(expected) if(argc != expected) { NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "ArgumentError"), "wrong number of arguments (given %d, expected %d)", argc, expected); }
#define NAT_ASSERT_ARGC2(expected_low, expected_high) if(argc < expected_low || argc > expected_high) { NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "ArgumentError"), "wrong number of arguments (given %d, expected %d..%d)", argc, expected_low, expected_high); }
#define NAT_ASSERT_ARGC_AT_LEAST(expected) if(argc < expected) { NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "ArgumentError"), "wrong number of arguments (given %d, expected %d+)", argc, expected); }
#define NAT_ASSERT_ARGC_AT_MOST(expected) if(argc > expected) { NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "ArgumentError"), "wrong number of arguments (given %d, expected 0..%d)", argc, expected); }
#define NAT_ASSERT_TYPE(obj, expected_type, expected_class_name) if (NAT_TYPE(obj) != expected_type) { NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "TypeError"), "no implicit conversion of %s into %s", obj->klass->class_name, expected_class_name); }
#define GET_MACRO(_1, _2, NAME, ...) NAME
#define NAT_ASSERT_ARGC(...) GET_MACRO(__VA_ARGS__, NAT_ASSERT_ARGC2, NAT_ASSERT_ARGC1)(__VA_ARGS__)
#define NAT_UNREACHABLE() fprintf(stderr, "panic: unreachable\n"); abort();
#define NAT_ASSERT_NOT_FROZEN(obj) if (nat_is_frozen(obj)) { NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "FrozenError"), "can't modify frozen %s", obj->klass->class_name); }
#define NAT_ASSERT_BLOCK() if (!block) { NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "ArgumentError"), "called without a block"); }
#define NAT_MIN(a, b) ((a < b) ? a : b)
#define NAT_MAX(a, b) ((a > b) ? a : b)
#define NAT_NOT_YET_IMPLEMENTED(description) fprintf(stderr, "NOT YET IMPLEMENTED: %s", description); abort();

// ahem, "globals"
#define NAT_OBJECT env->global_env->Object
#define NAT_INTEGER env->global_env->Integer
#define NAT_NIL env->global_env->nil
#define NAT_TRUE env->global_env->true_obj
#define NAT_FALSE env->global_env->false_obj

#define TRUE 1
#define FALSE 0

// only 63-bit numbers for now
#define NAT_MIN_INT INT64_MIN >> 1
#define NAT_MAX_INT INT64_MAX >> 1

#define NAT_INT_VALUE(obj) (((int64_t)obj & 1) ? ((int64_t)obj) >> 1 : obj->integer)

typedef struct NatObject NatObject;
typedef struct NatGlobalEnv NatGlobalEnv;
typedef struct NatEnv NatEnv;
typedef struct NatBlock NatBlock;
typedef struct NatMethod NatMethod;
typedef struct NatHashKeyListNode NatHashKeyListNode;
typedef struct NatHashKeyListNode NatHashIter;
typedef struct NatHashValueContainer NatHashValueContainer;

struct NatGlobalEnv {
    struct hashmap *globals;
    struct hashmap *symbols;
    NatObject *Object,
              *Integer,
              *nil,
              *true_obj,
              *false_obj;
};

struct NatEnv {
    NatGlobalEnv *global_env;
    size_t var_count;
    NatObject **vars;
    NatEnv *outer;
    int block;
    int rescue;
    jmp_buf jump_buf;
    NatObject *exception;
    NatEnv *caller;
    char *file;
    size_t line;
    char *method_name;
};

struct NatBlock {
    NatObject* (*fn)(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
    NatEnv env;
    NatObject *self;
};

struct NatMethod {
    NatObject* (*fn)(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
    NatEnv env;
};

struct NatHashKeyListNode {
    NatHashKeyListNode *prev;
    NatHashKeyListNode *next;
    NatObject *key;
    NatObject *val;
    int removed;
};

struct NatHashValueContainer {
    NatHashKeyListNode *key_list_node;
    NatObject *val;
};

enum NatValueType {
    NAT_VALUE_ARRAY, /// 0
    NAT_VALUE_CLASS,
    NAT_VALUE_EXCEPTION,
    NAT_VALUE_FALSE,
    NAT_VALUE_HASH,
    NAT_VALUE_INTEGER,
    NAT_VALUE_IO,
    NAT_VALUE_MATCHDATA,
    NAT_VALUE_MODULE,
    NAT_VALUE_NIL,
    NAT_VALUE_OTHER,
    NAT_VALUE_PROC,
    NAT_VALUE_RANGE,
    NAT_VALUE_REGEXP,
    NAT_VALUE_STRING,
    NAT_VALUE_SYMBOL,
    NAT_VALUE_TRUE
};

#define NAT_FLAG_MAIN_OBJECT 1
#define NAT_FLAG_FROZEN 2
#define NAT_FLAG_BREAK 4

#define nat_is_main_object(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT))

#define nat_is_frozen(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_FROZEN) == NAT_FLAG_FROZEN))
#define nat_freeze_object(obj) obj->flags = obj->flags | NAT_FLAG_FROZEN;

#define nat_flag_break(obj) obj = nat_convert_to_real_object(env, obj); obj->flags = obj->flags | NAT_FLAG_BREAK;
#define nat_is_break(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_BREAK) == NAT_FLAG_BREAK))
#define nat_remove_break(obj) ((obj)->flags = (obj)->flags & ~NAT_FLAG_BREAK)
#define nat_return_if_break(env, result) if (nat_is_break(result)) { nat_remove_break(result); return result; }

struct NatObject {
    enum NatValueType type;
    NatObject *klass;
    NatObject *singleton_class;
    int flags;

    NatEnv env;

    struct hashmap constants;
    struct hashmap ivars;
    
    union {
        int64_t integer;

        // NAT_VALUE_ARRAY
        struct {
            size_t ary_len;
            size_t ary_cap;
            NatObject **ary;
        };

        // NAT_VALUE_CLASS, NAT_VALUE_MODULE
        struct {
            char *class_name;
            NatObject *superclass;
            struct hashmap methods;
            size_t included_modules_count;
            NatObject **included_modules;
        };

        // NAT_VALUE_EXCEPTION
        struct {
            char *message;
            NatObject *backtrace; // array
        };

        // NAT_VALUE_HASHMAP
        struct {
            NatHashKeyListNode *key_list; // a double-ended queue
            struct hashmap hashmap;
        };

        // NAT_VALUE_IO
        int fileno;

        // NAT_VALUE_MATCHDATA
        struct {
            OnigRegion *matchdata_region;
            char *matchdata_str;
        };

        // NAT_VALUE_PROC
        struct {
            NatBlock *block;
            int lambda;
        };

        // NAT_VALUE_RANGE
        struct {
            NatObject *range_begin;
            NatObject *range_end;
            int range_exclude_end;
        };

        // NAT_VALUE_REGEXP
        struct {
            regex_t* regexp;
            char* regexp_str;
        };

        // NAT_VALUE_STRING
        struct {
            size_t str_len;
            size_t str_cap;
            char *str;
        };

        // NAT_VALUE_SYMBOL
        char *symbol;
    };
};

int nat_is_constant_name(char *name);
int nat_is_special_name(char *name);

NatObject *nat_const_get(NatEnv *env, NatObject *klass, char *name);
NatObject *nat_const_get_or_null(NatEnv *env, NatObject *klass, char *name);
NatObject *nat_const_set(NatEnv *env, NatObject *klass, char *name, NatObject *val);

NatObject *nat_var_get(NatEnv *env, char *key, size_t index);
NatObject *nat_var_set(NatEnv *env, char *key, size_t index, NatObject *val);
NatGlobalEnv *nat_build_global_env();
NatEnv *nat_build_env(NatEnv *env, NatEnv *outer);
NatEnv *nat_build_block_env(NatEnv *env, NatEnv *outer, NatEnv *calling_env);

char *nat_find_current_method_name(NatEnv *env);
char *nat_find_method_name(NatEnv *env);

NatObject* nat_raise(NatEnv *env, NatObject *klass, char *message_format, ...);
NatObject* nat_raise_exception(NatEnv *env, NatObject *exception);
NatObject* nat_raise_local_jump_error(NatEnv *env, NatObject *exit_value);

NatObject *nat_ivar_get(NatEnv *env, NatObject *obj, char *name);
NatObject *nat_ivar_set(NatEnv *env, NatObject *obj, char *name, NatObject *val);

NatObject *nat_global_get(NatEnv *env, char *name);
NatObject *nat_global_set(NatEnv *env, char *name, NatObject *val);

int nat_truthy(NatObject *obj);

char *heap_string(char *str);

NatObject *nat_alloc(NatEnv *env);
NatObject *nat_subclass(NatEnv *env, NatObject *superclass, char *name);
NatObject *nat_module(NatEnv *env, char *name);
void nat_class_include(NatObject *klass, NatObject *module);
NatObject *nat_new(NatEnv *env, NatObject *klass, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);

NatObject *nat_singleton_class(NatEnv *env, NatObject *obj);

NatObject *nat_integer(NatEnv *env, int64_t integer);

char *nat_object_pointer_id(NatObject *obj);

char* int_to_string(int64_t num);

void nat_define_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *block));
void nat_define_method_with_block(NatObject *obj, char *name, NatBlock *block);
void nat_define_singleton_method(NatEnv *env, NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *block));

NatObject *nat_class_ancestors(NatEnv *env, NatObject *klass);
int nat_is_a(NatEnv *env, NatObject *obj, NatObject *klass_or_module);

char *nat_defined(NatEnv *env, NatObject *receiver, char *name);
NatObject *nat_defined_obj(NatEnv *env, NatObject *receiver, char *name);

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args, NatBlock *block);
void nat_methods(NatEnv *env, NatObject *array, NatObject *klass);
NatMethod *nat_find_method(NatObject *klass, char *method_name, NatObject **matching_class_or_module);
NatObject *nat_call_method_on_class(NatEnv *env, NatObject *klass, NatObject *instance_class, char *method_name, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
int nat_respond_to(NatEnv *env, NatObject *obj, char *name);

NatBlock *nat_block(NatEnv *env, NatObject *self, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock*));
NatObject *nat_run_block(NatEnv *env, NatBlock *the_block, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block);
NatObject *nat_proc(NatEnv *env, NatBlock *block);
NatObject *nat_lambda(NatEnv *env, NatBlock *block);

#define NAT_STRING_GROW_FACTOR 2

NatObject *nat_string(NatEnv *env, char *str);
void nat_grow_string(NatObject *obj, size_t capacity);
void nat_grow_string_at_least(NatObject *obj, size_t min_capacity);
void nat_string_append(NatObject *str, char *s);
void nat_string_append_char(NatObject *str, char c);
void nat_string_append_nat_string(NatObject *str, NatObject *str2);
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
void nat_assign_arg(NatEnv *env, char *name, int argc, NatObject **args, int index, NatObject* default_value);
void nat_assign_rest_arg(NatEnv *env, char *name, size_t argc, NatObject **args, size_t index, size_t count);
void nat_array_push_splat(NatEnv *env, NatObject *array, NatObject *obj);
void nat_array_expand_with_nil(NatEnv *env, NatObject *array, size_t size);

NatObject *nat_convert_to_real_object(NatEnv *env, NatObject *obj);

NatHashKeyListNode *nat_hash_key_list_append(NatObject *hash, NatObject *key, NatObject *val);
void nat_hash_key_list_remove_node(NatObject *hash, NatHashKeyListNode *node);
NatHashIter *nat_hash_iter(NatEnv *env, NatObject *hash);
NatHashIter *nat_hash_iter_prev(NatEnv *env, NatObject *hash, NatHashIter *iter);
NatHashIter *nat_hash_iter_next(NatEnv *env, NatObject *hash, NatHashIter *iter);
size_t nat_hashmap_hash(const void *obj);
int nat_hashmap_compare(const void *a, const void *b);
NatObject *nat_hash(NatEnv *env);
NatObject *nat_hash_get(NatEnv *env, NatObject *hash, NatObject *key);
void nat_hash_put(NatEnv *env, NatObject *hash, NatObject *key, NatObject *val);
NatObject* nat_hash_delete(NatEnv *env, NatObject *hash, NatObject *key);

NatObject *nat_regexp(NatEnv *env, char *pattern);
NatObject *nat_matchdata(NatEnv *env, OnigRegion *region, NatObject *str_obj);

NatObject *nat_range(NatEnv *env, NatObject *begin, NatObject *end, int exclude_end);

NatObject *nat_dup(NatEnv *env, NatObject *obj);
NatObject *nat_not(NatEnv *env, NatObject *val);

void nat_alias(NatEnv *env, NatObject *self, char *new_name, char *old_name);

void nat_run_at_exit_handlers(NatEnv *env);
void nat_print_exception_with_backtrace(NatEnv *env, NatObject *exception);
void nat_handle_top_level_exception(NatEnv *env, int run_exit_handlers);

int64_t nat_object_id(NatEnv *env, NatObject *obj);

#endif
