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
#define NAT_TYPE(obj) (NAT_IS_TAGGED_INT(obj) ? Value::Type::Integer : obj->type)
#define NAT_IS_MODULE_OR_CLASS(obj) (NAT_TYPE(obj) == Value::Type::Module || NAT_TYPE(obj) == Value::Type::Class)
#define NAT_OBJ_CLASS(obj) (NAT_IS_TAGGED_INT(obj) ? NAT_INTEGER : obj->klass)
#define NAT_RESCUE(env) ((env->rescue = 1) && setjmp(env->jump_buf))

#define NAT_RAISE(env, class_name, message_format, ...)                                                      \
    {                                                                                                        \
        raise(env, const_get(env, NAT_OBJECT, class_name, true)->as_class(), message_format, ##__VA_ARGS__); \
        abort();                                                                                             \
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

#define NAT_INT_VALUE(obj) (((int64_t)obj & 1) ? RSHIFT((int64_t)obj, 1) : obj->as_integer()->to_int64_t())

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

// TODO: move to forward header
struct NilValue;
struct TrueValue;
struct FalseValue;
struct ArrayValue;
struct ModuleValue;
struct ClassValue;
struct EncodingValue;
struct ExceptionValue;
struct HashValue;
struct IntegerValue;
struct IoValue;
struct MatchDataValue;
struct ProcValue;
struct RangeValue;
struct RegexpValue;
struct StringValue;
struct SymbolValue;
struct VoidPValue;

struct GlobalEnv {
    GlobalEnv() {
        globals = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
        hashmap_init(globals, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(globals, hashmap_alloc_key_string, free);
        symbols = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
        hashmap_init(symbols, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(symbols, hashmap_alloc_key_string, free);
    }

    ~GlobalEnv() {
        hashmap_destroy(globals);
        hashmap_destroy(symbols);
    }

    struct hashmap *globals { nullptr };
    struct hashmap *symbols { nullptr };
    ClassValue *Object { nullptr };
    ClassValue *Integer { nullptr };
    NilValue *nil { nullptr };
    TrueValue *true_obj { nullptr };
    FalseValue *false_obj { nullptr };
};

struct Env {
    Env() { }

    Env(Env *outer)
        : outer { outer } {
        global_env = outer->global_env;
    }

    Env(GlobalEnv *global_env)
        : global_env { global_env } { }

    static Env new_block_env(Env *outer, Env *calling_env) {
        Env env(outer);
        env.block_env = true;
        env.caller = calling_env;
        return env;
    }

    static Env new_detatched_block_env(Env *outer) {
        Env env;
        env.global_env = outer->global_env;
        env.block_env = true;
        return env;
    }

    GlobalEnv *global_env { nullptr };
    Vector *vars { nullptr };
    Env *outer { nullptr };
    Block *block { nullptr };
    bool block_env { false };
    bool rescue { false };
    jmp_buf jump_buf;
    Value *exception { nullptr };
    Env *caller { nullptr };
    const char *file { nullptr };
    ssize_t line { 0 };
    const char *method_name { nullptr };
    Value *match { nullptr };
};

struct Block {
    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env;
    Value *self { nullptr };
};

struct Method {
    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env;
    bool undefined { false };
};

struct HashKey {
    HashKey *prev { nullptr };
    HashKey *next { nullptr };
    Value *key { nullptr };
    Value *val { nullptr };
    Env env;
    bool removed { false };
};

struct HashVal {
    HashKey *key { nullptr };
    Value *val { nullptr };
};

struct Vector {
    ssize_t size { 0 };
    ssize_t capacity { 0 };
    void **data { nullptr };
};

enum class Encoding {
    ASCII_8BIT = 1,
    UTF_8 = 2,
};

#define NAT_FLAG_MAIN_OBJECT 1
#define NAT_FLAG_FROZEN 2
#define NAT_FLAG_BREAK 4

#define is_main_object(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT))

#define is_frozen(obj) ((obj->is_integer() || ((obj)->flags & NAT_FLAG_FROZEN) == NAT_FLAG_FROZEN))

#define freeze_object(obj) obj->flags = obj->flags | NAT_FLAG_FROZEN;

#define flag_break(obj) \
    obj->flags = obj->flags | NAT_FLAG_BREAK;

#define is_break(obj) ((!NAT_IS_TAGGED_INT(obj) && ((obj)->flags & NAT_FLAG_BREAK) == NAT_FLAG_BREAK))

#define remove_break(obj) ((obj)->flags = (obj)->flags & ~NAT_FLAG_BREAK)

#define NAT_RUN_BLOCK_FROM_ENV(env, argc, args) ({                                      \
    Env *env_with_block = env;                                                          \
    while (!env_with_block->block && env_with_block->outer) {                           \
        env_with_block = env_with_block->outer;                                         \
    }                                                                                   \
    Value *_result = _run_block_internal(env, env_with_block->block, argc, args, NULL); \
    if (is_break(_result)) {                                                            \
        remove_break(_result);                                                          \
        return _result;                                                                 \
    }                                                                                   \
    _result;                                                                            \
})

#define NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, the_block, argc, args, block) ({ \
    Value *_result = _run_block_internal(env, the_block, argc, args, block);   \
    if (is_break(_result)) {                                                   \
        remove_break(_result);                                                 \
        return _result;                                                        \
    }                                                                          \
    _result;                                                                   \
})

#define NAT_RUN_BLOCK_WITHOUT_BREAK(env, the_block, argc, args, block) ({    \
    Value *_result = _run_block_internal(env, the_block, argc, args, block); \
    if (is_break(_result)) {                                                 \
        remove_break(_result);                                               \
        raise_local_jump_error(env, _result, "break from proc-closure");     \
        abort();                                                             \
    }                                                                        \
    _result;                                                                 \
})

#define EMPTY_HASHMAP \
    { 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr }

Value *const_get(Env *env, Value *klass, const char *name, bool strict);

struct Value {
    enum class Type {
        Nil,
        Array,
        Class,
        Encoding,
        Exception,
        False,
        Hash,
        Integer,
        Io,
        MatchData,
        Module,
        Object,
        Proc,
        Range,
        Regexp,
        String,
        Symbol,
        True,
        VoidP,
    };

    enum class Conversion {
        Strict,
        NullAllowed,
    };

    Type type { Type::Object };
    ClassValue *klass { nullptr };

    ClassValue *singleton_class { nullptr };

    ModuleValue *owner { nullptr };
    int flags { 0 };

    Env env;

    struct hashmap constants EMPTY_HASHMAP; // TODO: can any ol' object have constants???
    struct hashmap ivars EMPTY_HASHMAP;

    Value(Env *env)
        : klass { NAT_OBJECT } { assert(klass); }

    Value(Env *env, ClassValue *klass)
        : klass { klass } { assert(klass); }

    Value(Env *env, Type type, ClassValue *klass)
        : type { type }
        , klass { klass } { assert(klass); }

    bool is_nil() { return type == Type::Nil; }
    bool is_true() { return type == Type::True; }
    bool is_false() { return type == Type::False; }
    bool is_array() { return type == Type::Array; }
    bool is_module() { return type == Type::Module || type == Type::Class; }
    bool is_class() { return type == Type::Class; }
    bool is_encoding() { return type == Type::Encoding; }
    bool is_exception() { return type == Type::Exception; }
    bool is_hash() { return type == Type::Hash; }
    bool is_integer() { return type == Type::Integer; }
    bool is_io() { return type == Type::Io; }
    bool is_match_data() { return type == Type::MatchData; }
    bool is_proc() { return type == Type::Proc; }
    bool is_range() { return type == Type::Range; }
    bool is_regexp() { return type == Type::Regexp; }
    bool is_symbol() { return type == Type::Symbol; }
    bool is_string() { return type == Type::String; }
    bool is_void_p() { return type == Type::VoidP; }

    NilValue *as_nil();
    TrueValue *as_true();
    FalseValue *as_false();
    ArrayValue *as_array();
    ModuleValue *as_module();
    ClassValue *as_class();
    EncodingValue *as_encoding();
    ExceptionValue *as_exception();
    HashValue *as_hash();
    IntegerValue *as_integer();
    IoValue *as_io();
    MatchDataValue *as_match_data();
    ProcValue *as_proc();
    RangeValue *as_range();
    RegexpValue *as_regexp();
    StringValue *as_string();
    SymbolValue *as_symbol();
    VoidPValue *as_void_p();

    SymbolValue *to_symbol(Env *, Conversion);

    const char *symbol_or_string_to_str();

    virtual ~Value() { }
};

struct ModuleValue : Value {
    using Value::Value;

    ModuleValue(Env *env);

    char *class_name { nullptr };
    ClassValue *superclass { nullptr };
    struct hashmap methods EMPTY_HASHMAP;
    struct hashmap cvars EMPTY_HASHMAP;
    ssize_t included_modules_count { 0 };
    ModuleValue **included_modules { nullptr };
};

struct ClassValue : ModuleValue {
    using ModuleValue::ModuleValue;

    ClassValue(Env *env)
        : ModuleValue { env, Value::Type::Class, const_get(env, NAT_OBJECT, "Class", true)->as_class() } { }

    ClassValue(Env *env, ClassValue *klass)
        : ModuleValue { env, Value::Type::Class, klass } { }
};

struct NilValue : Value {
    static NilValue *instance(Env *env) {
        if (NAT_NIL) return NAT_NIL;
        return new NilValue { env };
    }

private:
    using Value::Value;

    NilValue(Env *env)
        : Value { env, Value::Type::Nil, const_get(env, NAT_OBJECT, "NilClass", true)->as_class() } { }
};

struct TrueValue : Value {
    static TrueValue *instance(Env *env) {
        if (NAT_TRUE) return NAT_TRUE;
        return new TrueValue { env };
    }

private:
    using Value::Value;

    TrueValue(Env *env)
        : Value { env, Value::Type::True, const_get(env, NAT_OBJECT, "TrueClass", true)->as_class() } { }
};

struct FalseValue : Value {
    static FalseValue *instance(Env *env) {
        if (NAT_FALSE) return NAT_FALSE;
        return new FalseValue { env };
    }

private:
    using Value::Value;

    FalseValue(Env *env)
        : Value { env, Value::Type::False, const_get(env, NAT_OBJECT, "FalseClass", true)->as_class() } { }
};

struct ArrayValue : Value {
    using Value::Value;

    ArrayValue(Env *env);

    Vector ary;
};

struct EncodingValue : Value {
    using Value::Value;

    EncodingValue(Env *env)
        : Value { env, Value::Type::Encoding, const_get(env, NAT_OBJECT, "Encoding", true)->as_class() } { }

    ArrayValue *encoding_names { nullptr };
    Encoding encoding_num;
};

struct ExceptionValue : Value {
    using Value::Value;

    ExceptionValue(Env *env)
        : Value { env, Value::Type::Exception, const_get(env, NAT_OBJECT, "Exception", true)->as_class() } { }

    ExceptionValue(Env *env, ClassValue *klass)
        : Value { env, Value::Type::Exception, klass } { }

    char *message { nullptr };
    ArrayValue *backtrace { nullptr };
};

struct HashValue : Value {
    using Value::Value;

    HashValue(Env *env)
        : Value { env, Value::Type::Hash, const_get(env, NAT_OBJECT, "Hash", true)->as_class() } { }

    HashKey *key_list { nullptr };
    struct hashmap hashmap EMPTY_HASHMAP;
    bool hash_is_iterating { false };
    Value *hash_default_value { nullptr };
    Block *hash_default_block { nullptr };
};

struct IntegerValue : Value {
    using Value::Value;

    IntegerValue(Env *env)
        : Value { env, Value::Type::Integer, const_get(env, NAT_OBJECT, "Integer", true)->as_class() } { }

    IntegerValue(Env *env, int64_t integer)
        : Value { env, Value::Type::Integer, const_get(env, NAT_OBJECT, "Integer", true)->as_class() }
        , integer { integer } { }

    int64_t integer { 0 };

    int64_t to_int64_t() {
        return integer;
    }

    bool is_zero() {
        return integer == 0;
    }
};

struct IoValue : Value {
    using Value::Value;

    IoValue(Env *env)
        : Value { env, Value::Type::Io, const_get(env, NAT_OBJECT, "IO", true)->as_class() } { }

    IoValue(Env *env, ClassValue *klass)
        : Value { env, Value::Type::Io, klass } { }

    int fileno { 0 };
};

struct MatchDataValue : Value {
    using Value::Value;

    MatchDataValue(Env *env)
        : Value { env, Value::Type::MatchData, const_get(env, NAT_OBJECT, "MatchData", true)->as_class() } { }

    OnigRegion *matchdata_region;
    char *matchdata_str { nullptr };
};

struct ProcValue : Value {
    using Value::Value;

    ProcValue(Env *env)
        : Value { env, Value::Type::Proc, const_get(env, NAT_OBJECT, "Proc", true)->as_class() } { }

    Block *block { nullptr };
    bool lambda;
};

struct RangeValue : Value {
    using Value::Value;

    RangeValue(Env *env)
        : Value { env, Value::Type::Range, const_get(env, NAT_OBJECT, "Range", true)->as_class() } { }

    Value *range_begin { nullptr };
    Value *range_end { nullptr };
    bool range_exclude_end { false };
};

struct RegexpValue : Value {
    using Value::Value;

    RegexpValue(Env *env)
        : Value { env, Value::Type::Regexp, const_get(env, NAT_OBJECT, "Regexp", true)->as_class() } { }

    regex_t *regexp { nullptr };
    char *regexp_str { nullptr };
};

struct StringValue : Value {
    using Value::Value;

    StringValue(Env *env)
        : Value { env, Value::Type::String, const_get(env, NAT_OBJECT, "String", true)->as_class() } { }

    StringValue(Env *env, ClassValue *klass)
        : Value { env, Value::Type::String, klass } { }

    ssize_t str_len { 0 };
    ssize_t str_cap { 0 };
    char *str { nullptr };
    Encoding encoding;

    SymbolValue *to_symbol(Env *env);
};

struct SymbolValue : Value {
    using Value::Value;

    SymbolValue(Env *env)
        : Value { env, Value::Type::Symbol, const_get(env, NAT_OBJECT, "Symbol", true)->as_class() } { }

    SymbolValue(Env *env, const char *symbol)
        : Value { env, Value::Type::Symbol, const_get(env, NAT_OBJECT, "Symbol", true)->as_class() }
        , symbol { symbol } { }

    const char *symbol { nullptr };
};

struct VoidPValue : Value {
    using Value::Value;

    VoidPValue(Env *env)
        : Value { env, Value::Type::VoidP, nullptr } { }

    void *void_ptr { nullptr };
};

bool is_constant_name(const char *name);

Value *const_get_or_null(Env *env, Value *klass, const char *name, bool strict, bool define);
Value *const_set(Env *env, Value *klass, const char *name, Value *val);

Value *var_get(Env *env, const char *key, ssize_t index);
Value *var_set(Env *env, const char *name, ssize_t index, bool allocate, Value *val);

const char *find_current_method_name(Env *env);

ExceptionValue *exception_new(Env *env, ClassValue *klass, const char *message);

Value *raise(Env *env, ClassValue *klass, const char *message_format, ...);
Value *raise_exception(Env *env, Value *exception);
Value *raise_exception(Env *env, ExceptionValue *exception);
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

ClassValue *subclass(Env *env, Value *superclass, const char *name);
ClassValue *subclass(Env *env, ClassValue *superclass, const char *name);
ModuleValue *module(Env *env, const char *name);
void class_include(Env *env, ModuleValue *klass, ModuleValue *module);
void class_prepend(Env *env, ModuleValue *klass, ModuleValue *module);

Value *initialize(Env *env, Value *obj, ssize_t argc, Value **args, Block *block);

ClassValue *singleton_class(Env *env, Value *obj);

IntegerValue *integer(Env *env, int64_t integer);

void int_to_string(int64_t num, char *buf);
void int_to_hex_string(int64_t num, char *buf, bool capitalize);

void define_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block));
void define_method_with_block(Env *env, Value *obj, const char *name, Block *block);
void define_singleton_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block));
void define_singleton_method_with_block(Env *env, Value *obj, const char *name, Block *block);
void undefine_method(Env *env, Value *obj, const char *name);
void undefine_singleton_method(Env *env, Value *obj, const char *name);

ArrayValue *class_ancestors(Env *env, ModuleValue *klass);
bool is_a(Env *env, Value *obj, Value *klass_or_module);
bool is_a(Env *env, Value *obj, ModuleValue *klass_or_module);

const char *defined(Env *env, Value *receiver, const char *name);
Value *defined_obj(Env *env, Value *receiver, const char *name);

Value *send(Env *env, Value *receiver, const char *sym, ssize_t argc, Value **args, Block *block);
void methods(Env *env, ArrayValue *array, ModuleValue *klass);
Method *find_method(ModuleValue *klass, const char *method_name, ModuleValue **matching_class_or_module);
Method *find_method_without_undefined(Value *klass, const char *method_name, ModuleValue **matching_class_or_module);
Value *call_begin(Env *env, Value *self, Value *(*block_fn)(Env *, Value *));
Value *call_method_on_class(Env *env, ClassValue *klass, Value *instance_class, const char *method_name, Value *self, ssize_t argc, Value **args, Block *block);
bool respond_to(Env *env, Value *obj, const char *name);

Block *block_new(Env *env, Value *self, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *));
Value *_run_block_internal(Env *env, Block *the_block, ssize_t argc, Value **args, Block *block);
ProcValue *proc_new(Env *env, Block *block);
ProcValue *to_proc(Env *env, Value *obj);
ProcValue *lambda(Env *env, Block *block);

#define NAT_STRING_GROW_FACTOR 2

StringValue *string_n(Env *env, const char *str, ssize_t len);
StringValue *string(Env *env, const char *str);
void grow_string(Env *env, StringValue *obj, ssize_t capacity);
void grow_string_at_least(Env *env, StringValue *obj, ssize_t min_capacity);
void string_append(Env *env, Value *val, const char *s);
void string_append(Env *env, StringValue *str, const char *s);
void string_append_char(Env *env, StringValue *str, char c);
void string_append_string(Env *env, Value *val, Value *val2);
void string_append_string(Env *env, StringValue *str, StringValue *str2);
ArrayValue *string_chars(Env *env, StringValue *str);
StringValue *sprintf(Env *env, const char *format, ...);
StringValue *vsprintf(Env *env, const char *format, va_list args);

SymbolValue *symbol(Env *env, const char *name);

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

ArrayValue *array_new(Env *env);
ArrayValue *array_with_vals(Env *env, ssize_t count, ...);
ArrayValue *array_copy(Env *env, ArrayValue *source);
void grow_array(Env *env, Value *obj, ssize_t capacity);
void grow_array_at_least(Env *env, Value *obj, ssize_t min_capacity);
void array_push(Env *env, Value *array, Value *obj);
void array_push(Env *env, ArrayValue *array, Value *obj);
void array_push_splat(Env *env, Value *array, Value *obj);
void array_push_splat(Env *env, ArrayValue *array, Value *obj);
void array_expand_with_nil(Env *env, Value *array, ssize_t size);
void array_expand_with_nil(Env *env, ArrayValue *array, ssize_t size);

Value *splat(Env *env, Value *obj);

HashKey *hash_key_list_append(Env *env, Value *hash, Value *key, Value *val);
void hash_key_list_remove_node(Value *hash, HashKey *node);
HashIter *hash_iter(Env *env, HashValue *hash);
HashIter *hash_iter_prev(Env *env, HashValue *hash, HashIter *iter);
HashIter *hash_iter_next(Env *env, HashValue *hash, HashIter *iter);
size_t hashmap_hash(const void *obj);
int hashmap_compare(const void *a, const void *b);
HashValue *hash_new(Env *env);
Value *hash_get(Env *env, Value *hash, Value *key);
Value *hash_get(Env *env, HashValue *hash, Value *key);
Value *hash_get_default(Env *env, HashValue *hash, Value *key);
void hash_put(Env *env, Value *hash, Value *key, Value *val);
void hash_put(Env *env, HashValue *hash, Value *key, Value *val);
Value *hash_delete(Env *env, HashValue *hash, Value *key);

RegexpValue *regexp_new(Env *env, const char *pattern);
MatchDataValue *matchdata_new(Env *env, OnigRegion *region, StringValue *str_obj);
Value *last_match(Env *env);

RangeValue *range_new(Env *env, Value *begin, Value *end, bool exclude_end);

Value *dup(Env *env, Value *obj);
Value *bool_not(Env *env, Value *val);

void alias(Env *env, Value *self, const char *new_name, const char *old_name);

void run_at_exit_handlers(Env *env);
void print_exception_with_backtrace(Env *env, ExceptionValue *exception);
void handle_top_level_exception(Env *env, bool run_exit_handlers);

void object_pointer_id(Value *obj, char *buf);
int64_t object_id(Env *env, Value *obj);

int quicksort_partition(Env *env, Value *ary[], int start, int end);
void quicksort(Env *env, Value *ary[], int start, int end);

ArrayValue *to_ary(Env *env, Value *obj, bool raise_for_non_array);

Value *arg_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...);
Value *array_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int offset_from_end, ssize_t path_size, ...);
Value *kwarg_value_by_name(Env *env, Value *args, const char *name, Value *default_value);
Value *kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, Value *default_value);

ArrayValue *args_to_array(Env *env, ssize_t argc, Value **args);
ArrayValue *block_args_to_array(Env *env, ssize_t signature_size, ssize_t argc, Value **args);

EncodingValue *encoding(Env *env, Encoding num, ArrayValue *names);

Value *eval_class_or_module_body(Env *env, Value *class_or_module, Value *(*fn)(Env *, Value *));

void arg_spread(Env *env, ssize_t argc, Value **args, const char *arrangement, ...);

VoidPValue *void_ptr(Env *env, void *ptr);

template <typename T>
void list_prepend(T *list, T item) {
    T next_item = *list;
    *list = item;
    item->next = next_item;
}

const char *str_from_string_or_symbol(Env *env, Value *string_or_symbol);
SymbolValue *symbol_from_symbol_or_string(Env *env, Value *symbol_or_string);
}
