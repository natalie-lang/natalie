#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

extern "C" {
#include "hashmap.h"
#include "onigmo.h"
}

struct Value : public gc {
    enum class Type {
        Nil,
        Array,
        Class,
        Encoding,
        Exception,
        False,
        Float,
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

    enum class ConstLookupSearchMode {
        NotStrict,
        Strict,
    };

    enum class ConstLookupFailureMode {
        Null,
        Raise,
    };

    Value(Env *env)
        : m_klass { env->Object() } { }

    Value(ClassValue *klass)
        : m_klass { klass } {
        assert(klass);
    }

    Value(Type type, ClassValue *klass)
        : m_klass { klass }
        , m_type { type } {
        assert(klass);
    }

    static Value *_new(Env *, Value *, ssize_t, Value **, Block *);

    Value(const Value &);

    Value &operator=(const Value &) = delete;

    Type type() { return m_type; }
    ClassValue *klass() { return m_klass; }

    ModuleValue *owner() { return m_owner; }
    void set_owner(ModuleValue *owner) { m_owner = owner; }

    int flags() { return m_flags; }

    Env env() { return m_env; }

    struct hashmap ivars() { // TODO: is this getter really needed?
        return m_ivars;
    }

    Value *initialize(Env *, ssize_t, Value **, Block *);

    bool is_nil() const { return m_type == Type::Nil; }
    bool is_true() const { return m_type == Type::True; }
    bool is_false() const { return m_type == Type::False; }
    bool is_array() const { return m_type == Type::Array; }
    bool is_module() const { return m_type == Type::Module || m_type == Type::Class; }
    bool is_class() const { return m_type == Type::Class; }
    bool is_encoding() const { return m_type == Type::Encoding; }
    bool is_exception() const { return m_type == Type::Exception; }
    bool is_float() const { return m_type == Type::Float; }
    bool is_hash() const { return m_type == Type::Hash; }
    bool is_integer() const { return m_type == Type::Integer; }
    bool is_io() const { return m_type == Type::Io; }
    bool is_match_data() const { return m_type == Type::MatchData; }
    bool is_proc() const { return m_type == Type::Proc; }
    bool is_range() const { return m_type == Type::Range; }
    bool is_regexp() const { return m_type == Type::Regexp; }
    bool is_symbol() const { return m_type == Type::Symbol; }
    bool is_string() const { return m_type == Type::String; }
    bool is_void_p() const { return m_type == Type::VoidP; }

    bool is_truthy() const { return !is_false() && !is_nil(); }
    bool is_falsey() const { return !is_truthy(); }
    bool is_numeric() const { return is_integer() || is_float(); }

    NilValue *as_nil();
    TrueValue *as_true();
    FalseValue *as_false();
    ArrayValue *as_array();
    ModuleValue *as_module();
    ClassValue *as_class();
    EncodingValue *as_encoding();
    ExceptionValue *as_exception();
    FloatValue *as_float();
    HashValue *as_hash();
    IntegerValue *as_integer();
    IoValue *as_io();
    FileValue *as_file();
    MatchDataValue *as_match_data();
    ProcValue *as_proc();
    RangeValue *as_range();
    RegexpValue *as_regexp();
    StringValue *as_string();
    SymbolValue *as_symbol();
    VoidPValue *as_void_p();

    KernelModule *as_kernel_module_for_method_binding();
    EnvValue *as_env_value_for_method_binding();
    ParserValue *as_parser_value_for_method_binding();

    SymbolValue *to_symbol(Env *, Conversion);

    const char *identifier_str(Env *, Conversion);

    ClassValue *singleton_class() { return m_singleton_class; }
    ClassValue *singleton_class(Env *);

    void set_singleton_class(ClassValue *c) { m_singleton_class = c; }

    virtual Value *const_get(const char *);
    virtual Value *const_fetch(const char *);
    virtual Value *const_find(Env *, const char *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise);
    virtual Value *const_set(Env *, const char *, Value *);

    Value *ivar_get(Env *, const char *);
    Value *ivar_set(Env *, const char *, Value *);
    Value *ivars(Env *);

    Value *cvar_get(Env *, const char *);
    virtual Value *cvar_get_or_null(Env *, const char *);
    virtual Value *cvar_set(Env *, const char *, Value *);

    virtual void define_method(Env *, const char *, Value *(*)(Env *, Value *, ssize_t, Value **, Block *block));
    virtual void define_method_with_block(Env *, const char *, Block *);
    virtual void undefine_method(Env *, const char *);

    void define_singleton_method(Env *, const char *, Value *(*)(Env *, Value *, ssize_t, Value **, Block *block));
    void define_singleton_method_with_block(Env *, const char *, Block *);
    void undefine_singleton_method(Env *, const char *);

    virtual void alias(Env *, const char *, const char *);

    int64_t object_id() { return (int64_t)this; }

    void pointer_id(char *buf) {
        snprintf(buf, NAT_OBJECT_POINTER_BUF_LENGTH, "%p", this);
    }

    Value *send(Env *, const char *, ssize_t = 0, Value ** = nullptr, Block * = nullptr);
    Value *send(Env *, ssize_t, Value **, Block *);

    Value *dup(Env *);

    bool is_a(Env *, Value *);
    bool respond_to(Env *, const char *);
    bool respond_to(Env *, Value *);

    const char *defined(Env *, const char *, bool);
    Value *defined_obj(Env *, const char *, bool = false);

    virtual ProcValue *to_proc(Env *);

    bool is_main_object() { return (m_flags & NAT_FLAG_MAIN_OBJECT) == NAT_FLAG_MAIN_OBJECT; }

    bool is_frozen() { return is_integer() || is_float() || (m_flags & NAT_FLAG_FROZEN) == NAT_FLAG_FROZEN; }
    void freeze() { m_flags = m_flags | NAT_FLAG_FROZEN; }

    void add_break_flag() { m_flags = m_flags | NAT_FLAG_BREAK; }
    void remove_break_flag() { m_flags = m_flags & ~NAT_FLAG_BREAK; }
    bool has_break_flag() { return (m_flags & NAT_FLAG_BREAK) == NAT_FLAG_BREAK; }

    void add_main_object_flag() { m_flags = m_flags | NAT_FLAG_MAIN_OBJECT; }

    bool eq(Env *, Value *other) {
        return this == other;
    }

    bool neq(Env *env, Value *other) {
        return send(env, "==", 1, &other)->is_falsey();
    }

    Value *instance_eval(Env *, Value *, Block *);

protected:
    Env m_env;
    ClassValue *m_klass { nullptr };

private:
    void init_ivars();

    Type m_type { Type::Object };

    ClassValue *m_singleton_class { nullptr };

    ModuleValue *m_owner { nullptr };
    int m_flags { 0 };

    struct hashmap m_ivars EMPTY_HASHMAP;
};

}
