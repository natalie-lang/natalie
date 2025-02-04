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

    enum class ConstLookupSearchMode {
        NotStrict,
        Strict,
        StrictPrivate,
    };

    enum class ConstLookupFailureMode {
        Null,
        Raise,
        ConstMissing,
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
        m_frozen = other.m_frozen;
        if (m_ivars)
            delete m_ivars;
        m_ivars = other.m_ivars;
        other.m_type = Type::Nil;
        other.m_singleton_class = nullptr;
        other.m_frozen = false;
        other.m_ivars = nullptr;
        return *this;
    }

    virtual ~Object() override {
        m_type = ObjectType::Collected;
        delete m_ivars;
    }

    static Value create(Env *, ClassObject *);
    static Value _new(Env *, Value, Args &&, Block *);
    static Value allocate(Env *, Value, Args &&, Block *);

    Type type() const { return m_type; }
    ClassObject *klass() const { return m_klass; }

    static Value initialize(Env *, Value);

    Enumerator::ArithmeticSequenceObject *as_enumerator_arithmetic_sequence();
    ArrayObject *as_array();
    const ArrayObject *as_array() const;
    BindingObject *as_binding();
    const BindingObject *as_binding() const;
    ClassObject *as_class();
    const ClassObject *as_class() const;
    ComplexObject *as_complex();
    const ComplexObject *as_complex() const;
    DirObject *as_dir();
    const DirObject *as_dir() const;
    EncodingObject *as_encoding();
    const EncodingObject *as_encoding() const;
    EnvObject *as_env();
    const EnvObject *as_env() const;
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
    ThreadObject *as_thread();
    const ThreadObject *as_thread() const;
    Thread::Backtrace::LocationObject *as_thread_backtrace_location();
    const Thread::Backtrace::LocationObject *as_thread_backtrace_location() const;
    ThreadGroupObject *as_thread_group();
    const ThreadGroupObject *as_thread_group() const;
    Thread::MutexObject *as_thread_mutex();
    const Thread::MutexObject *as_thread_mutex() const;
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
    EncodingObject *as_encoding_or_raise(Env *);
    ExceptionObject *as_exception_or_raise(Env *);
    FloatObject *as_float_or_raise(Env *);
    HashObject *as_hash_or_raise(Env *);
    MatchDataObject *as_match_data_or_raise(Env *);
    ModuleObject *as_module_or_raise(Env *);
    RangeObject *as_range_or_raise(Env *);
    StringObject *as_string_or_raise(Env *);

    static SymbolObject *to_instance_variable_name(Env *, Value);

    ClassObject *singleton_class() const { return m_singleton_class; }
    static ClassObject *singleton_class(Env *, Value);

    static ClassObject *subclass(Env *env, Value superklass, const char *name);

    void set_singleton_class(ClassObject *);

    void extend_once(Env *, ModuleObject *);

    static Value const_find_with_autoload(Env *, Value, Value, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::ConstMissing);
    static Value const_fetch(Value, SymbolObject *);
    static Value const_set(Env *, Value, SymbolObject *, Value);
    static Value const_set(Env *, Value, SymbolObject *, MethodFnPtr, StringObject *);

    static bool ivar_defined(Env *, Value, SymbolObject *);
    static Value ivar_get(Env *, Value, SymbolObject *);
    static Value ivar_set(Env *, Value, SymbolObject *, Value);

    bool ivar_defined(Env *, SymbolObject *);
    Value ivar_get(Env *, SymbolObject *);
    Value ivar_remove(Env *, SymbolObject *);
    Value ivar_set(Env *, SymbolObject *, Value);

    Value instance_variables(Env *);

    Value cvar_get(Env *, SymbolObject *);
    virtual Value cvar_get_or_raise(Env *, SymbolObject *);
    virtual Value cvar_get_or_null(Env *, SymbolObject *);
    virtual Value cvar_set(Env *, SymbolObject *, Value);

    static SymbolObject *define_method(Env *, Value, SymbolObject *, MethodFnPtr, int);
    static SymbolObject *define_method(Env *, Value, SymbolObject *, Block *);
    static SymbolObject *undefine_method(Env *, Value, SymbolObject *);

    static SymbolObject *define_singleton_method(Env *, Value, SymbolObject *, MethodFnPtr, int);
    static SymbolObject *define_singleton_method(Env *, Value, SymbolObject *, Block *);
    static SymbolObject *undefine_singleton_method(Env *, Value, SymbolObject *);

    Value main_obj_define_method(Env *, Value, Value, Block *);
    Value main_obj_inspect(Env *);

    virtual Value private_method(Env *, Args &&);
    virtual Value protected_method(Env *, Args &&);
    virtual Value module_function(Env *, Args &&);

    void private_method(Env *, SymbolObject *);
    void protected_method(Env *, SymbolObject *);
    void module_function(Env *, SymbolObject *);

    static void method_alias(Env *, Value, Value, Value);
    static void method_alias(Env *, Value, SymbolObject *, SymbolObject *);
    static void singleton_method_alias(Env *, Value, SymbolObject *, SymbolObject *);

    static nat_int_t object_id(const Value self) { return self.object_id(); }

    static Value itself(Value self) { return self; }

    String pointer_id() const {
        char buf[100]; // ought to be enough for anybody ;-)
        snprintf(buf, 100, "%p", this);
        return buf;
    }

    Value public_send(Env *, SymbolObject *, Args && = Args(), Block * = nullptr, Value sent_from = nullptr);
    static Value public_send(Env *, Value, Args &&, Block *);

    Value send(Env *, SymbolObject *, Args && = Args(), Block * = nullptr, Value sent_from = nullptr);
    static Value send(Env *, Value, Args &&, Block *);

    Value send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr, Value sent_from = nullptr) {
        // NOTE: sent_from is unused, but accepting it makes the SendInstruction codegen simpler. :-)
        return send(env, name, Args(args), block);
    }

    Value send(Env *, SymbolObject *, Args &&, Block *, MethodVisibility, Value = nullptr);
    Value method_missing_send(Env *, SymbolObject *, Args &&, Block *);
    static Value method_missing(Env *, Value, Args &&, Block *);

    Method *find_method(Env *, SymbolObject *, MethodVisibility, Value) const;

    Value duplicate(Env *) const;
    Value clone(Env *env, Value freeze = nullptr);
    static Value clone_obj(Env *env, Value self, Value freeze = nullptr);

    void copy_instance_variables(Value);

    bool respond_to(Env *env, SymbolObject *name) { return Value(this).respond_to(env, name); }

    const char *defined(Env *, SymbolObject *, bool);
    Value defined_obj(Env *, SymbolObject *, bool = false);

    virtual ProcObject *to_proc(Env *);

    bool is_main_object() const { return this == GlobalEnv::the()->main_obj(); }

    void freeze();
    bool is_frozen() const { return m_type == Type::Integer || m_type == Type::Float || m_frozen; }

    static bool not_truthy(Value self) {
        if (self.is_integer())
            return false;
        return self.is_falsey();
    }

    static bool eq(Env *, Value self, Value other) { return other == self; }
    static bool equal(Value, Value);
    static bool neq(Env *env, Value self, Value other);

    static Value instance_eval(Env *, Value, Args &&, Block *);
    static Value instance_exec(Env *, Value, Args &&, Block *);

    void assert_not_frozen(Env *);
    void assert_not_frozen(Env *, Value);

    String inspect_str(Env *env) { return Value(this).inspect_str(env); }

    Value enum_for(Env *env, const char *method, Args &&args = {});

    virtual void visit_children(Visitor &visitor) const override;

    virtual String dbg_inspect() const;

    virtual void gc_inspect(char *buf, size_t len) const override;

protected:
    ClassObject *m_klass { nullptr };

private:
    Type m_type { Type::Object };
    ClassObject *m_singleton_class { nullptr };
    bool m_frozen { false };
    TM::Hashmap<SymbolObject *, Value> *m_ivars { nullptr };
};

}
