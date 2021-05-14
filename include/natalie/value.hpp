#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/hashmap.hpp"
#include "natalie/macros.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/value_ptr.hpp"
#include "natalie/value_type.hpp"

namespace Natalie {

extern "C" {
#include "onigmo.h"
}

struct Value : public Cell {
    using Type = ValueType;

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

    Value(Env *, const Value &);

    Value(const Value &) = delete;

    Value &operator=(const Value &) = delete;

    virtual ~Value() override { }

    static ValuePtr _new(Env *, ValuePtr, size_t, ValuePtr *, Block *);

    Type type() { return m_type; }
    ClassValue *klass() { return m_klass; }

    ModuleValue *owner() { return m_owner; }
    void set_owner(ModuleValue *owner) { m_owner = owner; }

    int flags() { return m_flags; }

    ValuePtr initialize(Env *, size_t, ValuePtr *, Block *);

    bool is_nil() const { return m_type == Type::Nil; }
    bool is_true() const { return m_type == Type::True; }
    bool is_false() const { return m_type == Type::False; }
    bool is_fiber() const { return m_type == Type::Fiber; }
    bool is_array() const { return m_type == Type::Array; }
    bool is_method() const { return m_type == Type::Method; }
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

    ArrayValue *as_array();
    ClassValue *as_class();
    EncodingValue *as_encoding();
    ExceptionValue *as_exception();
    FalseValue *as_false();
    FiberValue *as_fiber();
    FileValue *as_file();
    FloatValue *as_float();
    HashValue *as_hash();
    IntegerValue *as_integer();
    IoValue *as_io();
    MatchDataValue *as_match_data();
    MethodValue *as_method();
    ModuleValue *as_module();
    NilValue *as_nil();
    ProcValue *as_proc();
    RangeValue *as_range();
    RegexpValue *as_regexp();
    StringValue *as_string();
    const StringValue *as_string() const;
    SymbolValue *as_symbol();
    TrueValue *as_true();
    VoidPValue *as_void_p();

    KernelModule *as_kernel_module_for_method_binding();
    EnvValue *as_env_value_for_method_binding();
    ParserValue *as_parser_value_for_method_binding();
    SexpValue *as_sexp_value_for_method_binding();

    SymbolValue *to_symbol(Env *, Conversion);

    const char *identifier_str(Env *, Conversion);

    ClassValue *singleton_class() { return m_singleton_class; }
    ClassValue *singleton_class(Env *);

    void set_singleton_class(ClassValue *);

    virtual ValuePtr const_get(Env *, SymbolValue *);
    virtual ValuePtr const_fetch(Env *, SymbolValue *);
    virtual ValuePtr const_find(Env *, SymbolValue *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise);
    virtual ValuePtr const_set(Env *, SymbolValue *, ValuePtr);

    ValuePtr ivar_get(Env *, SymbolValue *);
    ValuePtr ivar_set(Env *, SymbolValue *, ValuePtr);

    ValuePtr instance_variables(Env *);

    ValuePtr cvar_get(Env *, SymbolValue *);
    virtual ValuePtr cvar_get_or_null(Env *, SymbolValue *);
    virtual ValuePtr cvar_set(Env *, SymbolValue *, ValuePtr);

    virtual SymbolValue *define_method(Env *, SymbolValue *, MethodFnPtr, int arity);
    virtual SymbolValue *define_method(Env *, SymbolValue *, Block *);
    virtual SymbolValue *undefine_method(Env *, SymbolValue *);

    SymbolValue *define_singleton_method(Env *, SymbolValue *, MethodFnPtr, int);
    SymbolValue *define_singleton_method(Env *, SymbolValue *, Block *);
    SymbolValue *undefine_singleton_method(Env *, SymbolValue *);

    virtual void alias(Env *, SymbolValue *, SymbolValue *);

    nat_int_t object_id() { return reinterpret_cast<nat_int_t>(this); }

    const char *pointer_id() {
        char buf[100]; // ought to be enough for anybody ;-)
        snprintf(buf, 100, "%p", this);
        return strdup(buf);
    }

    ValuePtr _public_send(Env *, SymbolValue *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);

    ValuePtr _send(Env *, SymbolValue *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);
    ValuePtr _send(Env *, const char *, size_t = 0, ValuePtr * = nullptr, Block * = nullptr);
    ValuePtr _send(Env *, size_t, ValuePtr *, Block *);

    Method *find_method(Env *, SymbolValue *, MethodVisibility, ModuleValue ** = nullptr, Method * = nullptr);

    ValuePtr dup(Env *);

    bool is_a(Env *, ValuePtr);
    bool respond_to(Env *, const char *);
    bool respond_to(Env *, ValuePtr);

    const char *defined(Env *, SymbolValue *, bool);
    ValuePtr defined_obj(Env *, SymbolValue *, bool = false);

    virtual ProcValue *to_proc(Env *);

    bool is_main_object() { return (m_flags & Flag::MainObject) == Flag::MainObject; }
    void add_main_object_flag() { m_flags = m_flags | Flag::MainObject; }

    bool is_frozen() { return is_integer() || is_float() || (m_flags & Flag::Frozen) == Flag::Frozen; }
    void freeze() { m_flags = m_flags | Flag::Frozen; }

    void add_break_flag() { m_flags = m_flags | Flag::Break; }
    void remove_break_flag() { m_flags = m_flags & ~Flag::Break; }
    bool has_break_flag() { return (m_flags & Flag::Break) == Flag::Break; }

    bool eq(Env *, ValuePtr other) {
        return other == this;
    }

    bool neq(Env *env, ValuePtr other) {
        return _send(env, "==", 1, &other)->is_falsey();
    }

    ValuePtr instance_eval(Env *, ValuePtr, Block *);

    void assert_type(Env *, Value::Type, const char *);
    void assert_not_frozen(Env *);

    const char *inspect_str(Env *);

    ValuePtr enum_for(Env *env, const char *method, size_t argc = 0, ValuePtr *args = nullptr);

    virtual void visit_children(Visitor &visitor) override;

    virtual void gc_print() override {
        fprintf(stderr, "<Value %p type=%d>", this, (int)m_type);
    }

protected:
    ClassValue *m_klass { nullptr };

private:
    Type m_type { Type::Object };

    ClassValue *m_singleton_class { nullptr };

    ModuleValue *m_owner { nullptr };
    int m_flags { 0 };

    Hashmap<SymbolValue *, Value *> m_ivars {};
};

}
