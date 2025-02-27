#pragma once

#include <assert.h>
#include <functional>
#include <string.h>

#include "natalie/constant.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/method_info.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/object.hpp"
#include "tm/optional.hpp"

namespace Natalie {

using namespace TM;

class ModuleObject : public Object {
public:
    ModuleObject();
    ModuleObject(const char *);
    ModuleObject(Type, ClassObject *);

    ModuleObject(ClassObject *klass)
        : ModuleObject { Type::Module, klass } { }

    ModuleObject(const ModuleObject &other)
        : Object { other.type(), other.klass() }
        , m_constants { other.m_constants }
        , m_superclass { other.m_superclass }
        , m_owner { other.m_owner }
        , m_methods { other.m_methods }
        , m_class_vars { other.m_class_vars } {
        for (ModuleObject *module : const_cast<ModuleObject &>(other).m_included_modules) {
            m_included_modules.push(module);
        }
    }

    Object &operator=(Object &&other) = delete;

    Value initialize(Env *, Block *);

    Value include(Env *, Args &&args);
    void include_once(Env *, ModuleObject *);

    Value prepend(Env *, Args &&args);
    void prepend_once(Env *, ModuleObject *);

    Value extend_object(Env *, Value);

    Value is_autoload(Env *, Value) const;

    Optional<Value> const_find_with_autoload(Env *, Value, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::ConstMissing);
    Optional<Value> const_find(Env *, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::ConstMissing);

    Value const_get(SymbolObject *) const;
    Value const_get(Env *, Value, Optional<Value> = {});
    Value const_fetch(SymbolObject *) const;
    Value const_set(SymbolObject *, Value);
    Value const_set(SymbolObject *, MethodFnPtr, StringObject *);
    Value const_set(Env *, Value, Value);

    void remove_const(SymbolObject *);
    Value remove_const(Env *, Value);
    Value constants(Env *, Optional<Value>) const;
    Value const_missing(Env *, Value);

    void make_method_alias(Env *, SymbolObject *, SymbolObject *);
    void method_alias(Env *, SymbolObject *, SymbolObject *);

    Value eval_body(Env *, Value (*)(Env *, Value));

    Optional<String> name() const {
        return m_name;
    }

    void set_name(String name) {
        m_name = std::move(name);
    }

    void set_name(const char *name) {
        assert(name);
        m_name = String(name);
    }

    virtual ClassObject *superclass(Env *) { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassObject *superclass) { m_superclass = superclass; }

    ModuleObject *owner() const { return m_owner; }

    void included_modules(Env *, ArrayObject *);
    Value included_modules(Env *);
    const Vector<ModuleObject *> &included_modules() { return m_included_modules; }
    bool does_include_module(Env *, Value);

    virtual Optional<Value> cvar_get_maybe(Env *, SymbolObject *) override;
    virtual Value cvar_set(Env *, SymbolObject *, Value) override;
    bool class_variable_defined(Env *, Value);
    Value class_variable_get(Env *, Value);
    Value class_variable_set(Env *, Value, Value);
    ArrayObject *class_variables(Optional<Value> = {}) const;
    Value remove_class_variable(Env *, Value);

    Value define_method(Env *, Value, Optional<Value>, Block *);
    SymbolObject *define_method(Env *, SymbolObject *, MethodFnPtr, int);
    SymbolObject *define_method(Env *, SymbolObject *, Block *);
    SymbolObject *undefine_method(Env *, SymbolObject *);

    void methods(Env *, ArrayObject *, bool = true);
    void define_method(Env *, SymbolObject *, Method *, MethodVisibility);
    MethodInfo find_method(Env *, SymbolObject *, ModuleObject ** = nullptr, const Method ** = nullptr) const;
    MethodInfo find_method(Env *, SymbolObject *, const Method *) const;
    void assert_method_defined(Env *, SymbolObject *, MethodInfo);

    Value instance_method(Env *, Value);
    Value public_instance_method(Env *, Value);

    Value instance_methods(Env *, Optional<Value>, std::function<bool(MethodVisibility)>);
    Value instance_methods(Env *, Optional<Value>);
    Value private_instance_methods(Env *, Optional<Value>);
    Value protected_instance_methods(Env *, Optional<Value>);
    Value public_instance_methods(Env *, Optional<Value>);

    ArrayObject *ancestors(Env *);
    bool ancestors_includes(Env *, ModuleObject *);
    bool is_subclass_of(ModuleObject *);

    bool is_method_defined(Env *, Value) const;

    String inspect_str() const;
    Value inspect(Env *) const;
    String dbg_inspect() const override;
    Value name(Env *) const;
    Optional<String> name() { return m_name; }
    virtual String backtrace_name() const;

    ArrayObject *attr(Env *, Args &&);
    ArrayObject *attr_reader(Env *, Args &&);
    SymbolObject *attr_reader(Env *, Value);
    ArrayObject *attr_writer(Env *, Args &&);
    SymbolObject *attr_writer(Env *, Value);
    ArrayObject *attr_accessor(Env *, Args &&);

    static Value attr_reader_block_fn(Env *, Value, Args &&, Block *);
    static Value attr_writer_block_fn(Env *, Value, Args &&, Block *);

    Value module_eval(Env *, Block *);
    Value module_exec(Env *, Args &&, Block *);

    Value private_method(Env *, Args &&) override;
    Value protected_method(Env *, Args &&) override;
    Value public_method(Env *, Args &&);
    Value private_class_method(Env *, Args &&);
    Value public_class_method(Env *, Args &&);
    void set_method_visibility(Env *, Args &&, MethodVisibility);
    void set_method_visibility(Env *, SymbolObject *, MethodVisibility);
    Value module_function(Env *, Args &&) override;

    Value deprecate_constant(Env *, Args &&);
    Value private_constant(Env *, Args &&);
    Value public_constant(Env *, Args &&);

    bool const_defined(Env *, Value, Optional<Value> = {});
    Value alias_method(Env *, Value, Value);
    Value remove_method(Env *, Args &&);
    Value undef_method(Env *, Args &&);
    Value ruby2_keywords(Env *, Value);

    bool eqeqeq(Env *env, Value other) {
        return other.is_a(env, this);
    }

    virtual void visit_children(Visitor &) const override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        if (m_name)
            snprintf(buf, len, "<ModuleObject %p name=%s>", this, m_name.value().c_str());
        else
            snprintf(buf, len, "<ModuleObject %p name=(none)>", this);
    }

private:
    Optional<Value> handle_missing_constant(Env *, Value, ConstLookupFailureMode);

    ClassObject *as_class();

protected:
    Constant *find_constant(Env *, SymbolObject *, ModuleObject **, ConstLookupSearchMode = ConstLookupSearchMode::Strict);

    TM::Hashmap<SymbolObject *, Constant *> m_constants {};
    Optional<String> m_name {};
    ClassObject *m_superclass { nullptr };
    ModuleObject *m_owner { nullptr };
    TM::Hashmap<SymbolObject *, MethodInfo> m_methods {};
    TM::Hashmap<SymbolObject *, Optional<Value>> m_class_vars {};
    Vector<ModuleObject *> m_included_modules {};
    MethodVisibility m_method_visibility { MethodVisibility::Public };
    bool m_module_function { false };
};

}
