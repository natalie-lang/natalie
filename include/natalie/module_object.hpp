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
        , m_methods { other.m_methods }
        , m_class_vars { other.m_class_vars } {
        for (ModuleObject *module : const_cast<ModuleObject &>(other).m_included_modules) {
            m_included_modules.push(module);
        }
    }

    Value initialize(Env *, Block *);

    Value include(Env *, Args args);
    void include_once(Env *, ModuleObject *);

    Value prepend(Env *, Args args);
    void prepend_once(Env *, ModuleObject *);

    Value is_autoload(Env *, Value) const;

    virtual Value const_find(Env *, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise) override;
    virtual Value const_find_with_autoload(Env *, Value, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise) override;
    virtual Value const_get(SymbolObject *) const override;
    virtual Value const_fetch(SymbolObject *) override;
    virtual Value const_set(SymbolObject *, Value) override;
    virtual Value const_set(SymbolObject *, MethodFnPtr, StringObject *) override;

    Value const_get(Env *, Value, Value = nullptr);
    Value const_set(Env *, Value, Value);
    void remove_const(SymbolObject *);
    Value remove_const(Env *, Value);
    Value constants(Env *, Value) const;
    Value const_missing(Env *, Value);

    void make_method_alias(Env *, SymbolObject *, SymbolObject *);
    virtual void method_alias(Env *, SymbolObject *, SymbolObject *) override;

    Value eval_body(Env *, Value (*)(Env *, Value));

    Optional<String> class_name() const {
        return m_class_name;
    }

    void set_class_name(String name) {
        m_class_name = std::move(name);
    }

    void set_class_name(const char *name) {
        assert(name);
        m_class_name = String(name);
    }

    virtual ClassObject *superclass(Env *) { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassObject *superclass) { m_superclass = superclass; }

    void included_modules(Env *, ArrayObject *);
    Value included_modules(Env *);
    Vector<ModuleObject *> included_modules() { return m_included_modules; }
    bool does_include_module(Env *, Value);

    virtual Value cvar_get_or_null(Env *, SymbolObject *) override;
    virtual Value cvar_set(Env *, SymbolObject *, Value) override;
    bool class_variable_defined(Env *, Value);
    Value class_variable_get(Env *, Value);
    Value class_variable_set(Env *, Value, Value);
    ArrayObject *class_variables(Value = nullptr) const;
    Value remove_class_variable(Env *, Value);

    Value define_method(Env *, Value, Value, Block *);
    virtual SymbolObject *define_method(Env *, SymbolObject *, MethodFnPtr, int, bool optimized = false) override;
    virtual SymbolObject *define_method(Env *, SymbolObject *, Block *) override;
    virtual SymbolObject *undefine_method(Env *, SymbolObject *) override;

    void methods(Env *, ArrayObject *, bool = true);
    void define_method(Env *, SymbolObject *, Method *, MethodVisibility);
    MethodInfo find_method(Env *, SymbolObject *, ModuleObject ** = nullptr, const Method ** = nullptr) const;
    MethodInfo find_method(Env *, SymbolObject *, const Method *) const;
    void assert_method_defined(Env *, SymbolObject *, MethodInfo);

    Value instance_method(Env *, Value);
    Value public_instance_method(Env *, Value);

    Value instance_methods(Env *, Value, std::function<bool(MethodVisibility)>);
    Value instance_methods(Env *, Value);
    Value private_instance_methods(Env *, Value);
    Value protected_instance_methods(Env *, Value);
    Value public_instance_methods(Env *, Value);

    ArrayObject *ancestors(Env *);
    bool ancestors_includes(Env *, ModuleObject *);
    bool is_subclass_of(ModuleObject *);

    bool is_method_defined(Env *, Value) const;

    String inspect_str() const;
    Value inspect(Env *) const;
    String dbg_inspect() const override;
    Value name(Env *) const;
    Optional<String> name() { return m_class_name; }
    virtual String backtrace_name() const;

    ArrayObject *attr(Env *, Args);
    ArrayObject *attr_reader(Env *, Args);
    SymbolObject *attr_reader(Env *, Value);
    ArrayObject *attr_writer(Env *, Args);
    SymbolObject *attr_writer(Env *, Value);
    ArrayObject *attr_accessor(Env *, Args);

    static Value attr_reader_block_fn(Env *, Value, Args, Block *);
    static Value attr_writer_block_fn(Env *, Value, Args, Block *);

    Value module_eval(Env *, Block *);
    Value module_exec(Env *, Args, Block *);

    Value private_method(Env *, Args) override;
    Value protected_method(Env *, Args) override;
    Value public_method(Env *, Args);
    Value private_class_method(Env *, Args);
    Value public_class_method(Env *, Args);
    void set_method_visibility(Env *, Args, MethodVisibility);
    Value module_function(Env *, Args) override;

    Value deprecate_constant(Env *, Args);
    Value private_constant(Env *, Args);
    Value public_constant(Env *, Args);

    bool const_defined(Env *, Value, Value = nullptr);
    Value alias_method(Env *, Value, Value);
    Value remove_method(Env *, Args);
    Value undef_method(Env *, Args);
    Value ruby2_keywords(Env *, Value);

    bool eqeqeq(Env *env, Value other) {
        return other->is_a(env, this);
    }

    bool has_env() { return !!m_env; }
    Env *env() { return m_env; }

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        if (m_class_name)
            snprintf(buf, len, "<ModuleObject %p name=%s>", this, m_class_name.value().c_str());
        else
            snprintf(buf, len, "<ModuleObject %p name=(none)>", this);
    }

protected:
    Constant *find_constant(Env *, SymbolObject *, ModuleObject **, ConstLookupSearchMode = ConstLookupSearchMode::Strict);

    Env *m_env { nullptr };
    TM::Hashmap<SymbolObject *, Constant *> m_constants {};
    Optional<String> m_class_name {};
    ClassObject *m_superclass { nullptr };
    TM::Hashmap<SymbolObject *, MethodInfo> m_methods {};
    TM::Hashmap<SymbolObject *, Value> m_class_vars {};
    Vector<ModuleObject *> m_included_modules {};
    MethodVisibility m_method_visibility { MethodVisibility::Public };
    bool m_module_function { false };
};

}
