#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/object_type.hpp"
#include "natalie/value.hpp"
#include "tm/hashmap.hpp"

namespace Natalie {

extern "C" {
#include "onigmo.h"
}

class Object : public Cell {
public:
    using Type = ObjectType;

    enum Flag {
        // set on an object that cannot be modified --
        // note that Integer and Float are always frozen,
        // even if this flag is not set
        Frozen = 1,

        // set on an object returned from a block to signal
        // that `break` was called
        Break = 2,

        // set on an object returned from a block to signal
        // that `redo` was called
        Redo = 4,

        // set on an object to signal it only lives for a short time
        // on the stack, and not to capture it anywhere
        // (don't store in variables, arrays, hashes)
        Synthesized = 8,
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

    Object()
        : m_klass { GlobalEnv::the()->Object() } { }

    Object(ClassObject *klass)
        : m_klass { klass } {
        assert(klass);
    }

    Object(Type type, ClassObject *klass)
        : m_klass { klass }
        , m_type { type } {
        assert(klass);
    }

    Object(const Object &);

    Object &operator=(const Object &) = delete;

    Object &operator=(Object &&other) {
        m_type = other.m_type;
        m_singleton_class = other.m_singleton_class;
        m_owner = other.m_owner;
        m_flags = other.m_flags;
        m_ivars = other.m_ivars;
        other.m_type = Type::Nil;
        other.m_singleton_class = nullptr;
        other.m_owner = nullptr;
        other.m_flags = 0;
        other.m_ivars = nullptr;
        return *this;
    }

    virtual ~Object() override {
        m_type = ObjectType::Nil;
        delete m_ivars;
    }

    static Value create(Env *, ClassObject *);
    static Value _new(Env *, Value, Args, Block *);
    static Value allocate(Env *, Value, Args, Block *);

    Type type() const { return m_type; }
    ClassObject *klass() const { return m_klass; }

    ModuleObject *owner() const { return m_owner; }
    void set_owner(ModuleObject *owner) { m_owner = owner; }

    int flags() const { return m_flags; }

    Value initialize(Env *);

    bool is_nil() const { return m_type == Type::Nil; }
    bool is_true() const { return m_type == Type::True; }
    bool is_false() const { return m_type == Type::False; }
    bool is_fiber() const { return m_type == Type::Fiber; }
    bool is_enumerator_arithmetic_sequence() const { return m_type == Type::EnumeratorArithmeticSequence; }
    bool is_array() const { return m_type == Type::Array; }
    bool is_method() const { return m_type == Type::Method; }
    bool is_module() const { return m_type == Type::Module || m_type == Type::Class; }
    bool is_class() const { return m_type == Type::Class; }
    bool is_complex() const { return m_type == Type::Complex; }
    bool is_encoding() const { return m_type == Type::Encoding; }
    bool is_exception() const { return m_type == Type::Exception; }
    bool is_float() const { return m_type == Type::Float; }
    bool is_hash() const { return m_type == Type::Hash; }
    bool is_integer() const { return m_type == Type::Integer; }
    bool is_io() const { return m_type == Type::Io; }
    bool is_file_stat() const { return m_type == Type::FileStat; }
    bool is_match_data() const { return m_type == Type::MatchData; }
    bool is_proc() const { return m_type == Type::Proc; }
    bool is_random() const { return m_type == Type::Random; }
    bool is_range() const { return m_type == Type::Range; }
    bool is_rational() const { return m_type == Type::Rational; }
    bool is_regexp() const { return m_type == Type::Regexp; }
    bool is_symbol() const { return m_type == Type::Symbol; }
    bool is_string() const { return m_type == Type::String; }
    bool is_time() const { return m_type == Type::Time; }
    bool is_unbound_method() const { return m_type == Type::UnboundMethod; }
    bool is_void_p() const { return m_type == Type::VoidP; }

    bool is_truthy() const { return !is_false() && !is_nil(); }
    bool is_falsey() const { return !is_truthy(); }
    bool is_numeric() const { return is_integer() || is_float(); }
    bool is_boolean() const { return is_true() || is_false(); }

    Enumerator::ArithmeticSequenceObject *as_enumerator_arithmetic_sequence();
    ArrayObject *as_array();
    const ArrayObject *as_array() const;
    ClassObject *as_class();
    const ClassObject *as_class() const;
    ComplexObject *as_complex();
    const ComplexObject *as_complex() const;
    EncodingObject *as_encoding();
    const EncodingObject *as_encoding() const;
    ExceptionObject *as_exception();
    const ExceptionObject *as_exception() const;
    FalseObject *as_false();
    const FalseObject *as_false() const;
    FiberObject *as_fiber();
    const FiberObject *as_fiber() const;
    FileObject *as_file();
    const FileObject *as_file() const;
    FileStatObject *as_file_stat();
    const FileStatObject *as_file_stat() const;
    FloatObject *as_float();
    const FloatObject *as_float() const;
    HashObject *as_hash();
    const HashObject *as_hash() const;
    IntegerObject *as_integer();
    const IntegerObject *as_integer() const;
    IoObject *as_io();
    const IoObject *as_io() const;
    MatchDataObject *as_match_data();
    const MatchDataObject *as_match_data() const;
    MethodObject *as_method();
    const MethodObject *as_method() const;
    ModuleObject *as_module();
    const ModuleObject *as_module() const;
    NilObject *as_nil();
    const NilObject *as_nil() const;
    ProcObject *as_proc();
    const ProcObject *as_proc() const;
    RandomObject *as_random();
    const RandomObject *as_random() const;
    RangeObject *as_range();
    const RangeObject *as_range() const;
    RationalObject *as_rational();
    const RationalObject *as_rational() const;
    RegexpObject *as_regexp();
    const RegexpObject *as_regexp() const;
    StringObject *as_string();
    const StringObject *as_string() const;
    SymbolObject *as_symbol();
    const SymbolObject *as_symbol() const;
    TimeObject *as_time();
    const TimeObject *as_time() const;
    TrueObject *as_true();
    const TrueObject *as_true() const;
    UnboundMethodObject *as_unbound_method();
    const UnboundMethodObject *as_unbound_method() const;
    VoidPObject *as_void_p();
    const VoidPObject *as_void_p() const;

    ArrayObject *as_array_or_raise(Env *);
    ClassObject *as_class_or_raise(Env *);
    FloatObject *as_float_or_raise(Env *);
    HashObject *as_hash_or_raise(Env *);
    IntegerObject *as_integer_or_raise(Env *);
    StringObject *as_string_or_raise(Env *);

    KernelModule *as_kernel_module_for_method_binding();
    EnvObject *as_env_object_for_method_binding();
    ParserObject *as_parser_object_for_method_binding();
    SexpObject *as_sexp_object_for_method_binding();

    SymbolObject *to_symbol(Env *, Conversion);
    SymbolObject *to_instance_variable_name(Env *);

    ClassObject *singleton_class() const { return m_singleton_class; }
    ClassObject *singleton_class(Env *);

    ClassObject *subclass(Env *env, const char *name);

    void set_singleton_class(ClassObject *);

    Value extend(Env *, Args);
    void extend_once(Env *, ModuleObject *);

    virtual Value const_find(Env *, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise);
    virtual Value const_get(SymbolObject *) const;
    virtual Value const_fetch(SymbolObject *);
    virtual Value const_set(SymbolObject *, Value);

    bool ivar_defined(Env *, SymbolObject *);
    Value ivar_get(Env *, SymbolObject *);
    Value ivar_remove(Env *, SymbolObject *);
    Value ivar_set(Env *, SymbolObject *, Value);

    Value instance_variables(Env *);

    Value cvar_get(Env *, SymbolObject *);
    virtual Value cvar_get_or_null(Env *, SymbolObject *);
    virtual Value cvar_set(Env *, SymbolObject *, Value);

    virtual SymbolObject *define_method(Env *, SymbolObject *, MethodFnPtr, int, bool = false);
    virtual SymbolObject *define_method(Env *, SymbolObject *, Block *);
    virtual SymbolObject *undefine_method(Env *, SymbolObject *);

    SymbolObject *define_singleton_method(Env *, SymbolObject *, MethodFnPtr, int, bool = false);
    SymbolObject *define_singleton_method(Env *, SymbolObject *, Block *);
    SymbolObject *undefine_singleton_method(Env *, SymbolObject *);

    Value main_obj_define_method(Env *, Value, Value, Block *);

    virtual Value private_method(Env *, Args);
    virtual Value protected_method(Env *, Args);
    virtual Value module_function(Env *, Args);

    void private_method(Env *, SymbolObject *);
    void protected_method(Env *, SymbolObject *);
    void module_function(Env *, SymbolObject *);

    void alias(Env *env, Value new_name, Value old_name);
    virtual void alias(Env *, SymbolObject *, SymbolObject *);

    nat_int_t object_id() const { return reinterpret_cast<nat_int_t>(this); }

    Value itself() { return this; }

    String pointer_id() const {
        char buf[100]; // ought to be enough for anybody ;-)
        snprintf(buf, 100, "%p", this);
        return buf;
    }

    Value public_send(Env *, SymbolObject *, Args = {}, Block * = nullptr);
    Value public_send(Env *, Args, Block *);

    Value send(Env *, SymbolObject *, Args = {}, Block * = nullptr);
    Value send(Env *, Args, Block *);

    Value send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr) {
        return send(env, name, Args(args), block);
    }

    Value send(Env *, SymbolObject *, Args, Block *, MethodVisibility);
    Value method_missing(Env *, Args, Block *);

    Method *find_method(Env *, SymbolObject *, MethodVisibility);

    Value dup(Env *) const;
    Value clone(Env *env) const;

    bool is_a(Env *, Value) const;
    bool respond_to(Env *, Value, bool = true);
    bool respond_to_method(Env *, Value, Value) const;
    bool respond_to_method(Env *, Value, bool) const;

    const char *defined(Env *, SymbolObject *, bool);
    Value defined_obj(Env *, SymbolObject *, bool = false);

    virtual ProcObject *to_proc(Env *);

    bool is_main_object() const { return this == GlobalEnv::the()->main_obj(); }

    void freeze() { m_flags = m_flags | Flag::Frozen; }
    bool is_frozen() const { return is_integer() || is_float() || (m_flags & Flag::Frozen) == Flag::Frozen; }

    void add_synthesized_flag() { m_flags = m_flags | Flag::Synthesized; }
    bool is_synthesized() const { return (m_flags & Flag::Synthesized) == Flag::Synthesized; }

    void add_break_flag() { m_flags = m_flags | Flag::Break; }
    void remove_break_flag() { m_flags = m_flags & ~Flag::Break; }
    bool has_break_flag() const { return (m_flags & Flag::Break) == Flag::Break; }

    void add_redo_flag() { m_flags = m_flags | Flag::Redo; }
    void remove_redo_flag() { m_flags = m_flags & ~Flag::Redo; }
    bool has_redo_flag() const { return (m_flags & Flag::Redo) == Flag::Redo; }

    bool eq(Env *, Value other) {
        return other == this;
    }
    bool equal(Value);

    bool neq(Env *env, Value other);

    Value instance_eval(Env *, Value, Block *);

    void assert_type(Env *, Object::Type, const char *) const;
    void assert_not_frozen(Env *);

    String inspect_str(Env *);

    Value enum_for(Env *env, const char *method, Args args = {});

    virtual void visit_children(Visitor &visitor) override;

    virtual String dbg_inspect() const;

    virtual void gc_inspect(char *buf, size_t len) const override;

    ArrayObject *to_ary(Env *env);
    FloatObject *to_f(Env *env);
    IntegerObject *to_int(Env *env);
    StringObject *to_s(Env *env);
    StringObject *to_str(Env *env);

protected:
    ClassObject *m_klass { nullptr };

private:
    Type m_type { Type::Object };

    ClassObject *m_singleton_class { nullptr };

    ModuleObject *m_owner { nullptr };
    int m_flags { 0 };

    TM::Hashmap<SymbolObject *, Value> *m_ivars { nullptr };
};

}
