#pragma once

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace Natalie {

extern "C" {
#include "hashmap.h"
#include "onigmo.h"
}

//#define NAT_DEBUG_METHOD_RESOLUTION

#define NAT_IS_TAGGED_INT(obj) ((int64_t)obj & 1)
#define NAT_TYPE(obj) (NAT_IS_TAGGED_INT(obj) ? NAT_VALUE_INTEGER : obj->type)
#define NAT_IS_MODULE_OR_CLASS(obj) (NAT_TYPE(obj) == NAT_VALUE_MODULE || NAT_TYPE(obj) == NAT_VALUE_CLASS)
#define NAT_OBJ_CLASS(obj) (NAT_IS_TAGGED_INT(obj) ? NAT_INTEGER : obj->klass)
#define NAT_RESCUE(env) ((env->rescue = 1) && setjmp(env->jump_buf))

#define NAT_RAISE(env, class_name, message_format, ...)                                                  \
    {                                                                                                    \
        nat_raise(env, nat_const_get(env, NAT_OBJECT, class_name, true), message_format, ##__VA_ARGS__); \
        abort();                                                                                         \
    }

#define NAT_RAISE2(env, constant, message_format, ...)           \
    {                                                            \
        nat_raise(env, constant, message_format, ##__VA_ARGS__); \
        abort();                                                 \
    }

#define NAT_ASSERT_ARGC(...)                                       \
    NAT_GET_MACRO(__VA_ARGS__, NAT_ASSERT_ARGC2, NAT_ASSERT_ARGC1) \
    (__VA_ARGS__)

#define NAT_ASSERT_ARGC1(expected)                                                                            \
    if (argc != expected) {                                                                                   \
        NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, expected); \
    }

#define NAT_ASSERT_ARGC2(expected_low, expected_high)                                                                                \
    if (argc < expected_low || argc > expected_high) {                                                                               \
        NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d..%d)", argc, expected_low, expected_high); \
    }

#define NAT_ASSERT_ARGC_AT_LEAST(expected)                                                                     \
    if (argc < expected) {                                                                                     \
        NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d+)", argc, expected); \
    }

#define NAT_ASSERT_TYPE(obj, expected_type, expected_class_name)                                                                    \
    if (NAT_TYPE((obj)) != expected_type) {                                                                                         \
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into %s", NAT_OBJ_CLASS((obj))->class_name, expected_class_name); \
    }

#define NAT_GET_MACRO(_1, _2, NAME, ...) NAME

#define NAT_UNREACHABLE()                        \
    {                                            \
        fprintf(stderr, "panic: unreachable\n"); \
        abort();                                 \
    }

#define NAT_ASSERT_NOT_FROZEN(obj)                                                                                     \
    if (nat_is_frozen(obj)) {                                                                                          \
        NAT_RAISE(env, "FrozenError", "can't modify frozen %s: %S", NAT_OBJ_CLASS(obj)->class_name, NAT_INSPECT(obj)); \
    }

#define NAT_ASSERT_BLOCK()                                         \
    if (!block) {                                                  \
        NAT_RAISE(env, "ArgumentError", "called without a block"); \
    }

#define NAT_MIN(a, b) ((a < b) ? a : b)
#define NAT_MAX(a, b) ((a > b) ? a : b)

#define NAT_NOT_YET_IMPLEMENTED(description)                     \
    {                                                            \
        fprintf(stderr, "NOT YET IMPLEMENTED: %s", description); \
        abort();                                                 \
    }

#define NAT_OBJ_HAS_ENV(obj) ((obj)->env.global_env == env->global_env) // prefered check
#define NAT_OBJ_HAS_ENV2(obj) ((obj)->env.global_env) // limited check used when there is no current env, i.e. nat_hashmap_hash and nat_hashmap_compare

#define NAT_INSPECT(obj) nat_send(env, obj, "inspect", 0, NULL, NULL)

// ahem, "globals"
#define NAT_OBJECT env->global_env->Object
#define NAT_INTEGER env->global_env->Integer
#define NAT_NIL env->global_env->nil
#define NAT_TRUE env->global_env->true_obj
#define NAT_FALSE env->global_env->false_obj

// FIXME: Either use Autoconf or find another way to define this
#ifdef PORTABLE_RSHIFT
#define RSHIFT(x, y) (((x) < 0) ? ~((~(x)) >> y) : (x) >> y)
#else
#define RSHIFT(x, y) ((x) >> (int)y)
#endif

// only 63-bit numbers for now
#define NAT_MIN_INT RSHIFT(INT64_MIN, 1)
#define NAT_MAX_INT RSHIFT(INT64_MAX, 1)

#define NAT_INT_VALUE(obj) (((int64_t)obj & 1) ? RSHIFT((int64_t)obj, 1) : obj->integer)

#define NAT_INT_64_MAX_BUF_LEN 21 // 1 for sign, 19 for max digits, and 1 for null terminator

// "0x" + up to 16 hex chars + NULL terminator
#define NAT_OBJECT_POINTER_BUF_LENGTH 2 + 16 + 1

typedef struct NatObject NatObject;
typedef struct NatGlobalEnv NatGlobalEnv;
typedef struct NatEnv NatEnv;
typedef struct NatBlock NatBlock;
typedef struct NatMethod NatMethod;
typedef struct NatHashKey NatHashKey;
typedef struct NatHashKey NatHashIter;
typedef struct NatHashVal NatHashVal;
typedef struct NatVector NatVector;
typedef struct NatHeapBlock NatHeapBlock;
typedef struct NatHeapCell NatHeapCell;

struct NatHeapBlock;
struct NatHeapCell;

struct NatGlobalEnv {
    struct hashmap *globals;
    struct hashmap *symbols;
    NatObject *Object,
        *Integer,
        *nil,
        *true_obj,
        *false_obj;
    NatHeapBlock *heap;
    struct hashmap *heap_cells;
    int bytes_total;
    int bytes_available;
    void *bottom_of_stack;
    NatHeapCell *min_ptr;
    NatHeapCell *max_ptr;
    bool gc_enabled;
    NatEnv *top_env;
};

struct NatEnv {
    NatGlobalEnv *global_env;
    NatVector *vars;
    NatEnv *outer;
    NatBlock *block;
    bool block_env;
    bool rescue;
    jmp_buf jump_buf;
    NatObject *exception;
    NatEnv *caller;
    const char *file;
    ssize_t line;
    const char *method_name;
    NatObject *match;
};

struct NatBlock {
    NatObject *(*fn)(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block);
    NatEnv env;
    NatObject *self;
};

struct NatMethod {
    NatObject *(*fn)(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block);
    NatEnv env;
    bool undefined;
};

struct NatHashKey {
    NatHashKey *prev;
    NatHashKey *next;
    NatObject *key;
    NatObject *val;
    NatEnv env;
    bool removed;
};

struct NatHashVal {
    NatHashKey *key;
    NatObject *val;
};

struct NatVector {
    ssize_t size;
    ssize_t capacity;
    void **data;
};

enum NatValueType {
    NAT_VALUE_NIL = 0,
    NAT_VALUE_ARRAY = 1,
    NAT_VALUE_CLASS = 2,
    NAT_VALUE_ENCODING = 3,
    NAT_VALUE_EXCEPTION = 4,
    NAT_VALUE_FALSE = 5,
    NAT_VALUE_HASH = 6,
    NAT_VALUE_INTEGER = 7,
    NAT_VALUE_IO = 8,
    NAT_VALUE_MATCHDATA = 9,
    NAT_VALUE_MODULE = 10,
    NAT_VALUE_OTHER = 11,
    NAT_VALUE_PROC = 12,
    NAT_VALUE_RANGE = 13,
    NAT_VALUE_REGEXP = 14,
    NAT_VALUE_STRING = 15,
    NAT_VALUE_SYMBOL = 16,
    NAT_VALUE_TRUE = 17,
    NAT_VALUE_VOIDP = 18,
};

enum NatEncoding {
    NAT_ENCODING_ASCII_8BIT = 1,
    NAT_ENCODING_UTF_8 = 2,
};

#define NAT_FLAG_MAIN_OBJECT 1
#define NAT_FLAG_FROZEN 2
#define NAT_FLAG_BREAK 4

#define nat_is_main_object(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT))

#define nat_is_frozen(obj) ((NAT_IS_TAGGED_INT(obj) || ((obj)->flags & NAT_FLAG_FROZEN) == NAT_FLAG_FROZEN))

#define nat_freeze_object(obj) obj->flags = obj->flags | NAT_FLAG_FROZEN;

#define nat_flag_break(obj)                     \
    obj = nat_convert_to_real_object(env, obj); \
    obj->flags = obj->flags | NAT_FLAG_BREAK;

#define nat_is_break(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_BREAK) == NAT_FLAG_BREAK))

#define nat_remove_break(obj) ((obj)->flags = (obj)->flags & ~NAT_FLAG_BREAK)

#define NAT_RUN_BLOCK_FROM_ENV(env, argc, args) ({                                              \
    NatEnv *env_with_block = env;                                                               \
    while (!env_with_block->block && env_with_block->outer) {                                   \
        env_with_block = env_with_block->outer;                                                 \
    }                                                                                           \
    NatObject *_result = _nat_run_block_internal(env, env_with_block->block, argc, args, NULL); \
    if (nat_is_break(_result)) {                                                                \
        nat_remove_break(_result);                                                              \
        return _result;                                                                         \
    }                                                                                           \
    _result;                                                                                    \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, argc, args, block) ({       \
    NatObject *_result = _nat_run_block_internal(env, the_block, argc, args, block); \
    if (nat_is_break(_result)) {                                                     \
        nat_remove_break(_result);                                                   \
        return _result;                                                              \
    }                                                                                \
    _result;                                                                         \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, argc, args, block) ({            \
    NatObject *_result = _nat_run_block_internal(env, the_block, argc, args, block); \
    if (nat_is_break(_result)) {                                                     \
        nat_remove_break(_result);                                                   \
        nat_raise_local_jump_error(env, _result, "break from proc-closure");         \
        abort();                                                                     \
    }                                                                                \
    _result;                                                                         \
})

struct NatObject {
    enum NatValueType type;
    NatObject *klass;
    NatObject *singleton_class;
    NatObject *owner; // for contants, either a module or a class
    int flags;

    NatEnv env;

    struct hashmap constants;
    struct hashmap ivars;

    union {
        int64_t integer;

        // NAT_VALUE_ARRAY
        NatVector ary;

        // NAT_VALUE_CLASS, NAT_VALUE_MODULE
        struct {
            char *class_name;
            NatObject *superclass;
            struct hashmap methods;
            struct hashmap cvars;
            ssize_t included_modules_count;
            NatObject **included_modules;
        };

        // NAT_VALUE_ENCODING
        struct {
            NatObject *encoding_names; // array
            enum NatEncoding encoding_num; // should match NatEncoding enum above
        };

        // NAT_VALUE_EXCEPTION
        struct {
            char *message;
            NatObject *backtrace; // array
        };

        // NAT_VALUE_HASH
        struct {
            NatHashKey *key_list; // a double-ended queue
            struct hashmap hashmap;
            bool hash_is_iterating;
            NatObject *hash_default_value;
            NatBlock *hash_default_block;
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
            bool lambda;
        };

        // NAT_VALUE_RANGE
        struct {
            NatObject *range_begin;
            NatObject *range_end;
            bool range_exclude_end;
        };

        // NAT_VALUE_REGEXP
        struct {
            regex_t *regexp;
            char *regexp_str;
        };

        // NAT_VALUE_STRING
        struct {
            ssize_t str_len;
            ssize_t str_cap;
            char *str;
            enum NatEncoding encoding;
        };

        // NAT_VALUE_SYMBOL
        char *symbol;

        // NAT_VALUE_VOIDP
        void *void_ptr;
    };
};

bool nat_is_constant_name(const char *name);
bool nat_is_special_name(const char *name);

NatObject *nat_const_get(NatEnv *env, NatObject *klass, const char *name, bool strict);
NatObject *nat_const_get_or_null(NatEnv *env, NatObject *klass, const char *name, bool strict, bool define);
NatObject *nat_const_set(NatEnv *env, NatObject *klass, const char *name, NatObject *val);

NatObject *nat_var_get(NatEnv *env, const char *key, ssize_t index);
NatObject *nat_var_set(NatEnv *env, const char *name, ssize_t index, bool allocate, NatObject *val);

NatGlobalEnv *nat_build_global_env();
void nat_free_global_env(NatGlobalEnv *global_env);

NatEnv *nat_build_env(NatEnv *env, NatEnv *outer);
NatEnv *nat_build_block_env(NatEnv *env, NatEnv *outer, NatEnv *calling_env);
NatEnv *nat_build_detached_block_env(NatEnv *env, NatEnv *outer);

const char *nat_find_current_method_name(NatEnv *env);

NatObject *nat_raise(NatEnv *env, NatObject *klass, const char *message_format, ...);
NatObject *nat_raise_exception(NatEnv *env, NatObject *exception);
NatObject *nat_raise_local_jump_error(NatEnv *env, NatObject *exit_value, const char *message);

NatObject *nat_ivar_get(NatEnv *env, NatObject *obj, const char *name);
NatObject *nat_ivar_set(NatEnv *env, NatObject *obj, const char *name, NatObject *val);

NatObject *nat_cvar_get(NatEnv *env, NatObject *obj, const char *name);
NatObject *nat_cvar_get_or_null(NatEnv *env, NatObject *obj, const char *name);
NatObject *nat_cvar_set(NatEnv *env, NatObject *obj, const char *name, NatObject *val);

NatObject *nat_global_get(NatEnv *env, const char *name);
NatObject *nat_global_set(NatEnv *env, const char *name, NatObject *val);

bool nat_truthy(NatObject *obj);

char *heap_string(const char *str);

NatObject *nat_subclass(NatEnv *env, NatObject *superclass, const char *name);
NatObject *nat_module(NatEnv *env, const char *name);
void nat_class_include(NatEnv *env, NatObject *klass, NatObject *module);
void nat_class_prepend(NatEnv *env, NatObject *klass, NatObject *module);

NatObject *nat_initialize(NatEnv *env, NatObject *obj, ssize_t argc, NatObject **args, NatBlock *block);

NatObject *nat_singleton_class(NatEnv *env, NatObject *obj);

NatObject *nat_integer(NatEnv *env, int64_t integer);

void int_to_string(int64_t num, char *buf);
void int_to_hex_string(int64_t num, char *buf, bool capitalize);

void nat_define_method(NatEnv *env, NatObject *obj, const char *name, NatObject *(*fn)(NatEnv *, NatObject *, ssize_t, NatObject **, NatBlock *block));
void nat_define_method_with_block(NatEnv *env, NatObject *obj, const char *name, NatBlock *block);
void nat_define_singleton_method(NatEnv *env, NatObject *obj, const char *name, NatObject *(*fn)(NatEnv *, NatObject *, ssize_t, NatObject **, NatBlock *block));
void nat_define_singleton_method_with_block(NatEnv *env, NatObject *obj, const char *name, NatBlock *block);
void nat_undefine_method(NatEnv *env, NatObject *obj, const char *name);
void nat_undefine_singleton_method(NatEnv *env, NatObject *obj, const char *name);

NatObject *nat_class_ancestors(NatEnv *env, NatObject *klass);
bool nat_is_a(NatEnv *env, NatObject *obj, NatObject *klass_or_module);

const char *nat_defined(NatEnv *env, NatObject *receiver, const char *name);
NatObject *nat_defined_obj(NatEnv *env, NatObject *receiver, const char *name);

NatObject *nat_send(NatEnv *env, NatObject *receiver, const char *sym, ssize_t argc, NatObject **args, NatBlock *block);
void nat_methods(NatEnv *env, NatObject *array, NatObject *klass);
NatMethod *nat_find_method(NatObject *klass, const char *method_name, NatObject **matching_class_or_module);
NatMethod *nat_find_method_without_undefined(NatObject *klass, const char *method_name, NatObject **matching_class_or_module);
NatObject *nat_call_begin(NatEnv *env, NatObject *self, NatObject *(*block_fn)(NatEnv *, NatObject *));
NatObject *nat_call_method_on_class(NatEnv *env, NatObject *klass, NatObject *instance_class, const char *method_name, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block);
bool nat_respond_to(NatEnv *env, NatObject *obj, const char *name);

NatBlock *nat_block(NatEnv *env, NatObject *self, NatObject *(*fn)(NatEnv *, NatObject *, ssize_t, NatObject **, NatBlock *));
NatObject *_nat_run_block_internal(NatEnv *env, NatBlock *the_block, ssize_t argc, NatObject **args, NatBlock *block);
NatObject *nat_proc(NatEnv *env, NatBlock *block);
NatObject *nat_to_proc(NatEnv *env, NatObject *obj);
NatObject *nat_lambda(NatEnv *env, NatBlock *block);

#define NAT_STRING_GROW_FACTOR 2

NatObject *nat_string_n(NatEnv *env, const char *str, ssize_t len);
NatObject *nat_string(NatEnv *env, const char *str);
void nat_grow_string(NatEnv *env, NatObject *obj, ssize_t capacity);
void nat_grow_string_at_least(NatEnv *env, NatObject *obj, ssize_t min_capacity);
void nat_string_append(NatEnv *env, NatObject *str, const char *s);
void nat_string_append_char(NatEnv *env, NatObject *str, char c);
void nat_string_append_nat_string(NatEnv *env, NatObject *str, NatObject *str2);
NatObject *nat_string_chars(NatEnv *env, NatObject *str);
NatObject *nat_sprintf(NatEnv *env, const char *format, ...);
NatObject *nat_vsprintf(NatEnv *env, const char *format, va_list args);

NatObject *nat_symbol(NatEnv *env, const char *name);

NatObject *nat_exception(NatEnv *env, NatObject *klass, const char *message);

#define NAT_VECTOR_INIT_SIZE 10
#define NAT_VECTOR_GROW_FACTOR 2

NatVector *nat_vector(ssize_t capacity);
void nat_vector_init(NatVector *vec, ssize_t capacity);
ssize_t nat_vector_size(NatVector *vec);
ssize_t nat_vector_capacity(NatVector *vec);
void **nat_vector_data(NatVector *vec);
void *nat_vector_get(NatVector *vec, ssize_t index);
void nat_vector_set(NatVector *vec, ssize_t index, void *item);
void nat_vector_free(NatVector *vec);
void nat_vector_copy(NatVector *dest, NatVector *source);
void nat_vector_push(NatVector *vec, void *item);
void nat_vector_add(NatVector *new_vec, NatVector *vec1, NatVector *vec2);

NatObject *nat_array(NatEnv *env);
NatObject *nat_array_with_vals(NatEnv *env, ssize_t count, ...);
NatObject *nat_array_copy(NatEnv *env, NatObject *source);
void nat_grow_array(NatEnv *env, NatObject *obj, ssize_t capacity);
void nat_grow_array_at_least(NatEnv *env, NatObject *obj, ssize_t min_capacity);
void nat_array_push(NatEnv *env, NatObject *array, NatObject *obj);
void nat_array_push_splat(NatEnv *env, NatObject *array, NatObject *obj);
void nat_array_expand_with_nil(NatEnv *env, NatObject *array, ssize_t size);

NatObject *nat_splat(NatEnv *env, NatObject *obj);

NatHashKey *nat_hash_key_list_append(NatEnv *env, NatObject *hash, NatObject *key, NatObject *val);
void nat_hash_key_list_remove_node(NatObject *hash, NatHashKey *node);
NatHashIter *nat_hash_iter(NatEnv *env, NatObject *hash);
NatHashIter *nat_hash_iter_prev(NatEnv *env, NatObject *hash, NatHashIter *iter);
NatHashIter *nat_hash_iter_next(NatEnv *env, NatObject *hash, NatHashIter *iter);
size_t nat_hashmap_hash(const void *obj);
int nat_hashmap_compare(const void *a, const void *b);
NatObject *nat_hash(NatEnv *env);
NatObject *nat_hash_get(NatEnv *env, NatObject *hash, NatObject *key);
NatObject *nat_hash_get_default(NatEnv *env, NatObject *hash, NatObject *key);
void nat_hash_put(NatEnv *env, NatObject *hash, NatObject *key, NatObject *val);
NatObject *nat_hash_delete(NatEnv *env, NatObject *hash, NatObject *key);

NatObject *nat_regexp(NatEnv *env, const char *pattern);
NatObject *nat_matchdata(NatEnv *env, OnigRegion *region, NatObject *str_obj);
NatObject *nat_last_match(NatEnv *env);

NatObject *nat_range(NatEnv *env, NatObject *begin, NatObject *end, bool exclude_end);

NatObject *nat_dup(NatEnv *env, NatObject *obj);
NatObject *nat_not(NatEnv *env, NatObject *val);

void nat_alias(NatEnv *env, NatObject *self, const char *new_name, const char *old_name);

void nat_run_at_exit_handlers(NatEnv *env);
void nat_print_exception_with_backtrace(NatEnv *env, NatObject *exception);
void nat_handle_top_level_exception(NatEnv *env, bool run_exit_handlers);

void nat_object_pointer_id(NatObject *obj, char *buf);
int64_t nat_object_id(NatEnv *env, NatObject *obj);

NatObject *nat_convert_to_real_object(NatEnv *env, NatObject *obj);

int nat_quicksort_partition(NatEnv *env, NatObject *ary[], int start, int end);
void nat_quicksort(NatEnv *env, NatObject *ary[], int start, int end);

NatObject *nat_to_ary(NatEnv *env, NatObject *obj, bool raise_for_non_array);

NatObject *nat_arg_value_by_path(NatEnv *env, NatObject *value, NatObject *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...);
NatObject *nat_array_value_by_path(NatEnv *env, NatObject *value, NatObject *default_value, bool splat, int offset_from_end, ssize_t path_size, ...);
NatObject *nat_kwarg_value_by_name(NatEnv *env, NatObject *args, const char *name, NatObject *default_value);

NatObject *nat_args_to_array(NatEnv *env, ssize_t argc, NatObject **args);
NatObject *nat_block_args_to_array(NatEnv *env, ssize_t signature_size, ssize_t argc, NatObject **args);

NatObject *nat_encoding(NatEnv *env, enum NatEncoding num, NatObject *names);

NatObject *nat_eval_class_or_module_body(NatEnv *env, NatObject *class_or_module, NatObject *(*fn)(NatEnv *, NatObject *));

void nat_arg_spread(NatEnv *env, ssize_t argc, NatObject **args, const char *arrangement, ...);

NatObject *nat_void_ptr(NatEnv *env, void *ptr);

template <typename T>
void nat_list_prepend(T* list, T item) {
    T next_item = *list;
    *list = item;
    item->next = next_item;
}

}
