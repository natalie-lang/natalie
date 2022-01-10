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
        MainObject = 1,
        Frozen = 2,
        Break = 4,
        Redo = 8,
        Inspecting = 16,
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
        m_ivars = std::move(other.m_ivars);
        return *this;
    }

    virtual ~Object() override {
        m_type = ObjectType::Nil;
    }

    static Value create(Env *, ClassObject *);
    static Value _new(Env *, Value, size_t, Value *, Block *);
    static Value allocate(Env *, Value, size_t, Value *, Block *);

    Type type() { return m_type; }
    ClassObject *klass() const { return m_klass; }

    ModuleObject *owner() { return m_owner; }
    void set_owner(ModuleObject *owner) { m_owner = owner; }

    int flags() const { return m_flags; }

    Value initialize(Env *, size_t, Value *, Block *);

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
    bool is_random() const { return m_type == Type::Random; }
    bool is_range() const { return m_type == Type::Range; }
    bool is_regexp() const { return m_type == Type::Regexp; }
    bool is_symbol() const { return m_type == Type::Symbol; }
    bool is_string() const { return m_type == Type::String; }
    bool is_unbound_method() const { return m_type == Type::UnboundMethod; }
    bool is_void_p() const { return m_type == Type::VoidP; }

    bool is_truthy() const { return !is_false() && !is_nil(); }
    bool is_falsey() const { return !is_truthy(); }
    bool is_numeric() const { return is_integer() || is_float(); }
    bool is_boolean() const { return is_true() || is_false(); }

    ArrayObject *as_array();
    ClassObject *as_class();
    EncodingObject *as_encoding();
    ExceptionObject *as_exception();
    FalseObject *as_false();
    FiberObject *as_fiber();
    FileObject *as_file();
    FloatObject *as_float();
    HashObject *as_hash();
    IntegerObject *as_integer();
    IoObject *as_io();
    MatchDataObject *as_match_data();
    MethodObject *as_method();
    ModuleObject *as_module();
    NilObject *as_nil();
    ProcObject *as_proc();
    RandomObject *as_random();
    RangeObject *as_range();
    RegexpObject *as_regexp();
    StringObject *as_string();
    const StringObject *as_string() const;
    SymbolObject *as_symbol();
    TrueObject *as_true();
    UnboundMethodObject *as_unbound_method();
    VoidPObject *as_void_p();

    KernelModule *as_kernel_module_for_method_binding();
    EnvObject *as_env_object_for_method_binding();
    ParserObject *as_parser_object_for_method_binding();
    SexpObject *as_sexp_object_for_method_binding();

    SymbolObject *to_symbol(Env *, Conversion);
    SymbolObject *to_instance_variable_name(Env *);

    const String *identifier_str(Env *, Conversion);

    ClassObject *singleton_class() const { return m_singleton_class; }
    ClassObject *singleton_class(Env *);

    void set_singleton_class(ClassObject *);

    Value extend(Env *, size_t, Value *);
    void extend_once(Env *, ModuleObject *);

    virtual Value const_find(Env *, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise);
    virtual Value const_get(SymbolObject *);
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

    virtual SymbolObject *define_method(Env *, SymbolObject *, MethodFnPtr, int arity);
    virtual SymbolObject *define_method(Env *, SymbolObject *, Block *);
    virtual SymbolObject *undefine_method(Env *, SymbolObject *);

    SymbolObject *define_singleton_method(Env *, SymbolObject *, MethodFnPtr, int);
    SymbolObject *define_singleton_method(Env *, SymbolObject *, Block *);
    SymbolObject *undefine_singleton_method(Env *, SymbolObject *);

    virtual Value private_method(Env *, size_t, Value *);
    virtual Value protected_method(Env *, size_t, Value *);
    virtual Value module_function(Env *, size_t, Value *);

    void private_method(Env *, SymbolObject *);
    void protected_method(Env *, SymbolObject *);
    void module_function(Env *, SymbolObject *);

    virtual void alias(Env *, SymbolObject *, SymbolObject *);

    nat_int_t object_id() { return reinterpret_cast<nat_int_t>(this); }

    Value itself() { return this; }

    const String *pointer_id() {
        char buf[100]; // ought to be enough for anybody ;-)
        snprintf(buf, 100, "%p", this);
        return new String(buf);
    }

    Value public_send(Env *, SymbolObject *, size_t = 0, Value * = nullptr, Block * = nullptr);

    Value send(Env *, SymbolObject *, size_t = 0, Value * = nullptr, Block * = nullptr);
    Value send(Env *, size_t, Value *, Block *);

    Value send(Env *env, SymbolObject *name, std::initializer_list<Value> args, Block *block = nullptr) {
        return send(env, name, args.size(), const_cast<Value *>(data(args)), block);
    }

    Method *find_method(Env *, SymbolObject *, MethodVisibility);

    Value dup(Env *);

    bool is_a(Env *, Value) const;
    bool respond_to(Env *, Value, bool = true);
    bool respond_to_method(Env *, Value, Value) const;
    bool respond_to_method(Env *, Value, bool) const;

    const char *defined(Env *, SymbolObject *, bool);
    Value defined_obj(Env *, SymbolObject *, bool = false);

    virtual ProcObject *to_proc(Env *);

    bool is_main_object() const { return (m_flags & Flag::MainObject) == Flag::MainObject; }
    void add_main_object_flag() { m_flags = m_flags | Flag::MainObject; }

    bool is_frozen() const { return is_integer() || is_float() || (m_flags & Flag::Frozen) == Flag::Frozen; }
    void freeze() { m_flags = m_flags | Flag::Frozen; }

    void add_break_flag() { m_flags = m_flags | Flag::Break; }
    void remove_break_flag() { m_flags = m_flags & ~Flag::Break; }
    bool has_break_flag() const { return (m_flags & Flag::Break) == Flag::Break; }

    void add_redo_flag() { m_flags = m_flags | Flag::Redo; }
    void remove_redo_flag() { m_flags = m_flags & ~Flag::Redo; }
    bool has_redo_flag() const { return (m_flags & Flag::Redo) == Flag::Redo; }

    void add_inspecting_flag() { m_flags = m_flags | Flag::Inspecting; }
    void remove_inspecting_flag() { m_flags = m_flags & ~Flag::Inspecting; }
    bool has_inspecting_flag() const { return (m_flags & Flag::Inspecting) == Flag::Inspecting; }

    bool eq(Env *, Value other) {
        return other == this;
    }
    bool equal(Value);

    bool neq(Env *env, Value other);

    Value instance_eval(Env *, Value, Block *);

    void assert_type(Env *, Object::Type, const char *);
    void assert_not_frozen(Env *);

    const String *inspect_str(Env *);

    Value enum_for(Env *env, const char *method, size_t argc = 0, Value *args = nullptr);

    virtual void visit_children(Visitor &visitor) override;

    virtual void gc_inspect(char *buf, size_t len) const override;

    ArrayObject *to_ary(Env *env);

protected:
    ClassObject *m_klass { nullptr };

private:
    Type m_type { Type::Object };

    ClassObject *m_singleton_class { nullptr };

    ModuleObject *m_owner { nullptr };
    int m_flags { 0 };

    TM::Hashmap<SymbolObject *, Value> m_ivars {};
};

}
