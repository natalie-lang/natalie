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

#define NAT_RAISE(env, class_name, message_format, ...)                                          \
    {                                                                                            \
        raise(env, const_get(env, NAT_OBJECT, class_name, true), message_format, ##__VA_ARGS__); \
        abort();                                                                                 \
    }

#define NAT_RAISE2(env, constant, message_format, ...)       \
    {                                                        \
        raise(env, constant, message_format, ##__VA_ARGS__); \
        abort();                                             \
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
    if (is_frozen(obj)) {                                                                                              \
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
#define NAT_OBJ_HAS_ENV2(obj) ((obj)->env.global_env) // limited check used when there is no current env, i.e. hashmap_hash and hashmap_compare

#define NAT_INSPECT(obj) send(env, obj, "inspect", 0, NULL, NULL)

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

typedef struct Value Value;
typedef struct GlobalEnv GlobalEnv;
typedef struct Env Env;
typedef struct Block Block;
typedef struct Method Method;
typedef struct HashKey HashKey;
typedef struct HashKey HashIter;
typedef struct HashVal HashVal;
typedef struct Vector Vector;
typedef struct HeapBlock HeapBlock;
typedef struct HeapCell HeapCell;

struct HeapBlock;
struct HeapCell;

struct GlobalEnv {
    struct hashmap *globals;
    struct hashmap *symbols;
    Value *Object,
        *Integer,
        *nil,
        *true_obj,
        *false_obj;
    HeapBlock *heap;
    struct hashmap *heap_cells;
    int bytes_total;
    int bytes_available;
    void *bottom_of_stack;
    HeapCell *min_ptr;
    HeapCell *max_ptr;
    bool gc_enabled;
    Env *top_env;
};

struct Env {
    GlobalEnv *global_env;
    Vector *vars;
    Env *outer;
    Block *block;
    bool block_env;
    bool rescue;
    jmp_buf jump_buf;
    Value *exception;
    Env *caller;
    const char *file;
    ssize_t line;
    const char *method_name;
    Value *match;
};

struct Block {
    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
    Env env;
    Value *self;
};

struct Method {
    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block);
    Env env;
    bool undefined;
};

struct HashKey {
    HashKey *prev;
    HashKey *next;
    Value *key;
    Value *val;
    Env env;
    bool removed;
};

struct HashVal {
    HashKey *key;
    Value *val;
};

struct Vector {
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

#define is_main_object(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT))

#define is_frozen(obj) ((NAT_IS_TAGGED_INT(obj) || ((obj)->flags & NAT_FLAG_FROZEN) == NAT_FLAG_FROZEN))

#define freeze_object(obj) obj->flags = obj->flags | NAT_FLAG_FROZEN;

#define flag_break(obj)                     \
    obj = convert_to_real_object(env, obj); \
    obj->flags = obj->flags | NAT_FLAG_BREAK;

#define is_break(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_BREAK) == NAT_FLAG_BREAK))

#define remove_break(obj) ((obj)->flags = (obj)->flags & ~NAT_FLAG_BREAK)

#define NAT_RUN_BLOCK_FROM_ENV(env, argc, args) ({                                              \
    Env *env_with_block = env;                                                               \
    while (!env_with_block->block && env_with_block->outer) {                                   \
        env_with_block = env_with_block->outer;                                                 \
    }                                                                                           \
    Value *_result = _run_block_internal(env, env_with_block->block, argc, args, NULL); \
    if (is_break(_result)) {                                                                \
        remove_break(_result);                                                              \
        return _result;                                                                         \
    }                                                                                           \
    _result;                                                                                    \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, argc, args, block) ({       \
    Value *_result = _run_block_internal(env, the_block, argc, args, block); \
    if (is_break(_result)) {                                                     \
        remove_break(_result);                                                   \
        return _result;                                                              \
    }                                                                                \
    _result;                                                                         \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, argc, args, block) ({            \
    Value *_result = _run_block_internal(env, the_block, argc, args, block); \
    if (is_break(_result)) {                                                     \
        remove_break(_result);                                                   \
        raise_local_jump_error(env, _result, "break from proc-closure");         \
        abort();                                                                     \
    }                                                                                \
    _result;                                                                         \
})

struct Value {
    enum NatValueType type;
    Value *klass;
    Value *singleton_class;
    Value *owner; // for contants, either a module or a class
    int flags;

    Env env;

    struct hashmap constants;
    struct hashmap ivars;

    union {
        int64_t integer;

        // NAT_VALUE_ARRAY
        Vector ary;

        // NAT_VALUE_CLASS, NAT_VALUE_MODULE
        struct {
            char *class_name;
            Value *superclass;
            struct hashmap methods;
            struct hashmap cvars;
            ssize_t included_modules_count;
            Value **included_modules;
        };

        // NAT_VALUE_ENCODING
        struct {
            Value *encoding_names; // array
            enum NatEncoding encoding_num; // should match NatEncoding enum above
        };

        // NAT_VALUE_EXCEPTION
        struct {
            char *message;
            Value *backtrace; // array
        };

        // NAT_VALUE_HASH
        struct {
            HashKey *key_list; // a double-ended queue
            struct hashmap hashmap;
            bool hash_is_iterating;
            Value *hash_default_value;
            Block *hash_default_block;
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
            Block *block;
            bool lambda;
        };

        // NAT_VALUE_RANGE
        struct {
            Value *range_begin;
            Value *range_end;
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

bool is_constant_name(const char *name);
bool is_special_name(const char *name);

Value *const_get(Env *env, Value *klass, const char *name, bool strict);
Value *const_get_or_null(Env *env, Value *klass, const char *name, bool strict, bool define);
Value *const_set(Env *env, Value *klass, const char *name, Value *val);

Value *var_get(Env *env, const char *key, ssize_t index);
Value *var_set(Env *env, const char *name, ssize_t index, bool allocate, Value *val);

GlobalEnv *build_global_env();
void free_global_env(GlobalEnv *global_env);

Env *build_env(Env *env, Env *outer);
Env *build_block_env(Env *env, Env *outer, Env *calling_env);
Env *build_detached_block_env(Env *env, Env *outer);

const char *find_current_method_name(Env *env);

Value *exception_new(Env *env, Value *klass, const char *message);

Value *raise(Env *env, Value *klass, const char *message_format, ...);
Value *raise_exception(Env *env, Value *exception);
Value *raise_local_jump_error(Env *env, Value *exit_value, const char *message);

Value *ivar_get(Env *env, Value *obj, const char *name);
Value *ivar_set(Env *env, Value *obj, const char *name, Value *val);

Value *cvar_get(Env *env, Value *obj, const char *name);
Value *cvar_get_or_null(Env *env, Value *obj, const char *name);
Value *cvar_set(Env *env, Value *obj, const char *name, Value *val);

Value *global_get(Env *env, const char *name);
Value *global_set(Env *env, const char *name, Value *val);

bool truthy(Value *obj);

char *heap_string(const char *str);

Value *subclass(Env *env, Value *superclass, const char *name);
Value *module(Env *env, const char *name);
void class_include(Env *env, Value *klass, Value *module);
void class_prepend(Env *env, Value *klass, Value *module);

Value *initialize(Env *env, Value *obj, ssize_t argc, Value **args, Block *block);

Value *singleton_class(Env *env, Value *obj);

Value *integer(Env *env, int64_t integer);

void int_to_string(int64_t num, char *buf);
void int_to_hex_string(int64_t num, char *buf, bool capitalize);

void define_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block));
void define_method_with_block(Env *env, Value *obj, const char *name, Block *block);
void define_singleton_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block));
void define_singleton_method_with_block(Env *env, Value *obj, const char *name, Block *block);
void undefine_method(Env *env, Value *obj, const char *name);
void undefine_singleton_method(Env *env, Value *obj, const char *name);

Value *class_ancestors(Env *env, Value *klass);
bool is_a(Env *env, Value *obj, Value *klass_or_module);

const char *defined(Env *env, Value *receiver, const char *name);
Value *defined_obj(Env *env, Value *receiver, const char *name);

Value *send(Env *env, Value *receiver, const char *sym, ssize_t argc, Value **args, Block *block);
void methods(Env *env, Value *array, Value *klass);
Method *find_method(Value *klass, const char *method_name, Value **matching_class_or_module);
Method *find_method_without_undefined(Value *klass, const char *method_name, Value **matching_class_or_module);
Value *call_begin(Env *env, Value *self, Value *(*block_fn)(Env *, Value *));
Value *call_method_on_class(Env *env, Value *klass, Value *instance_class, const char *method_name, Value *self, ssize_t argc, Value **args, Block *block);
bool respond_to(Env *env, Value *obj, const char *name);

Block *block_new(Env *env, Value *self, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *));
Value *_run_block_internal(Env *env, Block *the_block, ssize_t argc, Value **args, Block *block);
Value *proc_new(Env *env, Block *block);
Value *to_proc(Env *env, Value *obj);
Value *lambda(Env *env, Block *block);

#define NAT_STRING_GROW_FACTOR 2

Value *string_n(Env *env, const char *str, ssize_t len);
Value *string(Env *env, const char *str);
void grow_string(Env *env, Value *obj, ssize_t capacity);
void grow_string_at_least(Env *env, Value *obj, ssize_t min_capacity);
void string_append(Env *env, Value *str, const char *s);
void string_append_char(Env *env, Value *str, char c);
void string_append_string(Env *env, Value *str, Value *str2);
Value *string_chars(Env *env, Value *str);
Value *sprintf(Env *env, const char *format, ...);
Value *vsprintf(Env *env, const char *format, va_list args);

Value *symbol(Env *env, const char *name);

#define NAT_VECTOR_INIT_SIZE 10
#define NAT_VECTOR_GROW_FACTOR 2

Vector *vector(ssize_t capacity);
void vector_init(Vector *vec, ssize_t capacity);
ssize_t vector_size(Vector *vec);
ssize_t vector_capacity(Vector *vec);
void **vector_data(Vector *vec);
void *vector_get(Vector *vec, ssize_t index);
void vector_set(Vector *vec, ssize_t index, void *item);
void vector_free(Vector *vec);
void vector_copy(Vector *dest, Vector *source);
void vector_push(Vector *vec, void *item);
void vector_add(Vector *new_vec, Vector *vec1, Vector *vec2);

Value *array_new(Env *env);
Value *array_with_vals(Env *env, ssize_t count, ...);
Value *array_copy(Env *env, Value *source);
void grow_array(Env *env, Value *obj, ssize_t capacity);
void grow_array_at_least(Env *env, Value *obj, ssize_t min_capacity);
void array_push(Env *env, Value *array, Value *obj);
void array_push_splat(Env *env, Value *array, Value *obj);
void array_expand_with_nil(Env *env, Value *array, ssize_t size);

Value *splat(Env *env, Value *obj);

HashKey *hash_key_list_append(Env *env, Value *hash, Value *key, Value *val);
void hash_key_list_remove_node(Value *hash, HashKey *node);
HashIter *hash_iter(Env *env, Value *hash);
HashIter *hash_iter_prev(Env *env, Value *hash, HashIter *iter);
HashIter *hash_iter_next(Env *env, Value *hash, HashIter *iter);
size_t hashmap_hash(const void *obj);
int hashmap_compare(const void *a, const void *b);
Value *hash_new(Env *env);
Value *hash_get(Env *env, Value *hash, Value *key);
Value *hash_get_default(Env *env, Value *hash, Value *key);
void hash_put(Env *env, Value *hash, Value *key, Value *val);
Value *hash_delete(Env *env, Value *hash, Value *key);

Value *regexp(Env *env, const char *pattern);
Value *matchdata(Env *env, OnigRegion *region, Value *str_obj);
Value *last_match(Env *env);

Value *range(Env *env, Value *begin, Value *end, bool exclude_end);

Value *dup(Env *env, Value *obj);
Value *bool_not(Env *env, Value *val);

void alias(Env *env, Value *self, const char *new_name, const char *old_name);

void run_at_exit_handlers(Env *env);
void print_exception_with_backtrace(Env *env, Value *exception);
void handle_top_level_exception(Env *env, bool run_exit_handlers);

void object_pointer_id(Value *obj, char *buf);
int64_t object_id(Env *env, Value *obj);

Value *convert_to_real_object(Env *env, Value *obj);

int quicksort_partition(Env *env, Value *ary[], int start, int end);
void quicksort(Env *env, Value *ary[], int start, int end);

Value *to_ary(Env *env, Value *obj, bool raise_for_non_array);

Value *arg_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...);
Value *array_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int offset_from_end, ssize_t path_size, ...);
Value *kwarg_value_by_name(Env *env, Value *args, const char *name, Value *default_value);

Value *args_to_array(Env *env, ssize_t argc, Value **args);
Value *block_args_to_array(Env *env, ssize_t signature_size, ssize_t argc, Value **args);

Value *encoding(Env *env, enum NatEncoding num, Value *names);

Value *eval_class_or_module_body(Env *env, Value *class_or_module, Value *(*fn)(Env *, Value *));

void arg_spread(Env *env, ssize_t argc, Value **args, const char *arrangement, ...);

Value *void_ptr(Env *env, void *ptr);

template <typename T>
void list_prepend(T* list, T item) {
    T next_item = *list;
    *list = item;
    item->next = next_item;
}

}
