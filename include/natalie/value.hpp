#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/hashmap.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

extern "C" {
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
        Fiber,
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

    enum Flag {
        MainObject = 1,
        Frozen = 2,
        Break = 4,
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

    Value(Value &other)
        : m_klass { other.m_klass }
        , m_type { other.m_type } {
        if (other.m_ivars.table) {
            init_ivars();
            struct hashmap_iter *iter;
            for (iter = hashmap_iter(&other.m_ivars); iter; iter = hashmap_iter_next(&other.m_ivars, iter)) {
                char *name = (char *)hashmap_iter_get_key(iter);
                hashmap_put(&m_ivars, name, hashmap_iter_get_data(iter));
            }
        }
    }

    virtual ~Value() { }

    static ValuePtr _new(Env *, ValuePtr , size_t, ValuePtr *, Block *);

    Value(const Value &);

    Value &operator=(const Value &) = delete;

    Type type() { return m_type; }
    ClassValue *klass() { return m_klass; }

    ModuleValue *owner() { return m_owner; }
    void set_owner(ModuleValue *owner) { m_owner = owner; }

    int flags() { return m_flags; }

    Env env() { return m_env; }

    ValuePtr initialize(Env *, size_t, ValuePtr *, Block *);

    bool is_nil() const { return m_type == Type::Nil; }
    bool is_true() const { return m_type == Type::True; }
    bool is_false() const { return m_type == Type::False; }
    bool is_fiber() const { return m_type == Type::Fiber; }
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
    FiberValue *as_fiber();
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
    SexpValue *as_sexp_value_for_method_binding();

    SymbolValue *to_symbol(Env *, Conversion);

    const char *identifier_str(Env *, Conversion);

    ClassValue *singleton_class() { return m_singleton_class; }
    ClassValue *singleton_class(Env *);

    void set_singleton_class(ClassValue *c) { m_singleton_class = c; }

    virtual ValuePtr const_get(const char *);
    virtual ValuePtr const_fetch(const char *);
    virtual ValuePtr const_find(Env *, const char *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise);
    virtual ValuePtr const_set(Env *, const char *, ValuePtr );

    ValuePtr ivar_get(Env *, const char *);
    ValuePtr ivar_set(Env *, const char *, ValuePtr );

    ValuePtr instance_variables(Env *);

    ValuePtr cvar_get(Env *, const char *);
    virtual ValuePtr cvar_get_or_null(Env *, const char *);
    virtual ValuePtr cvar_set(Env *, const char *, ValuePtr );

    virtual void define_method(Env *, const char *, MethodFnPtr);
    virtual void define_method_with_block(Env *, const char *, Block *);
    virtual void undefine_method(Env *, const char *);

    void define_singleton_method(Env *, const char *, ValuePtr (*)(Env *, ValuePtr , size_t, ValuePtr *, Block *block));
    void define_singleton_method_with_block(Env *, const char *, Block *);
    void undefine_singleton_method(Env *, const char *);

    virtual void alias(Env *, const char *, const char *);

    nat_int_t object_id() { return reinterpret_cast<nat_int_t>(this); }

    const char *pointer_id() {
        char buf[100]; // ought to be enough for anybody ;-)
        snprintf(buf, 100, "%p", this);
        return GC_STRDUP(buf);
    }

    ValuePtr send(Env *, const char *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);
    ValuePtr send(Env *, size_t, ValuePtr *, Block *);

    ValuePtr dup(Env *);

    bool is_a(Env *, ValuePtr );
    bool respond_to(Env *, const char *);
    bool respond_to(Env *, ValuePtr );

    const char *defined(Env *, const char *, bool);
    ValuePtr defined_obj(Env *, const char *, bool = false);

    virtual ProcValue *to_proc(Env *);

    bool is_main_object() { return (m_flags & Flag::MainObject) == Flag::MainObject; }
    void add_main_object_flag() { m_flags = m_flags | Flag::MainObject; }

    bool is_frozen() { return is_integer() || is_float() || (m_flags & Flag::Frozen) == Flag::Frozen; }
    void freeze() { m_flags = m_flags | Flag::Frozen; }

    void add_break_flag() { m_flags = m_flags | Flag::Break; }
    void remove_break_flag() { m_flags = m_flags & ~Flag::Break; }
    bool has_break_flag() { return (m_flags & Flag::Break) == Flag::Break; }

    bool eq(Env *, ValuePtr other) {
        return this == other;
    }

    bool neq(Env *env, ValuePtr other) {
        return send(env, "==", 1, &other)->is_falsey();
    }

    ValuePtr instance_eval(Env *, ValuePtr , Block *);

    void assert_type(Env *, Value::Type, const char *);
    void assert_not_frozen(Env *);

    const char *inspect_str(Env *);

protected:
    Env m_env;
    ClassValue *m_klass { nullptr };

private:
    void init_ivars();

    Type m_type { Type::Object };

    ClassValue *m_singleton_class { nullptr };

    ModuleValue *m_owner { nullptr };
    int m_flags { 0 };

    hashmap m_ivars {};
};

}
