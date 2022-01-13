#pragma once

#include <assert.h>
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

    ModuleObject(ModuleObject &other)
        : Object { other.type(), other.klass() }
        , m_constants { other.m_constants }
        , m_class_name { other.m_class_name }
        , m_superclass { other.m_superclass }
        , m_methods { other.m_methods } {
        for (ModuleObject *module : const_cast<ModuleObject &>(other).m_included_modules) {
            m_included_modules.push(module);
        }
    }

    Value initialize(Env *, Block *);

    Value include(Env *, size_t argc, Value *args);
    void include_once(Env *, ModuleObject *);

    Value prepend(Env *, size_t argc, Value *args);
    void prepend_once(Env *, ModuleObject *);

    virtual Value const_find(Env *env, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise) override;
    virtual Value const_get(SymbolObject *) override;
    virtual Value const_fetch(SymbolObject *) override;
    virtual Value const_set(SymbolObject *, Value) override;

    Value const_set(Env *, Value, Value);

    virtual void alias(Env *, SymbolObject *, SymbolObject *) override;

    Value eval_body(Env *, Value (*)(Env *, Value));

    Optional<const String *> class_name() {
        return m_class_name;
    }

    void set_class_name(const String *name) {
        assert(name);
        m_class_name = name;
    }

    void set_class_name(const char *name) {
        assert(name);
        m_class_name = new String(name);
    }

    virtual ClassObject *superclass(Env *) { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassObject *superclass) { m_superclass = superclass; }

    Value included_modules(Env *);
    Vector<ModuleObject *> included_modules() { return m_included_modules; }
    bool does_include_module(Env *, Value);

    virtual Value cvar_get_or_null(Env *, SymbolObject *) override;
    virtual Value cvar_set(Env *, SymbolObject *, Value) override;

    Value define_method(Env *, Value, Value, Block *);
    virtual SymbolObject *define_method(Env *, SymbolObject *, MethodFnPtr, int) override;
    virtual SymbolObject *define_method(Env *, SymbolObject *, Block *) override;
    virtual SymbolObject *undefine_method(Env *, SymbolObject *) override;

    void methods(Env *, ArrayObject *, bool = true);
    void define_method(Env *, SymbolObject *, Method *, MethodVisibility);
    void set_method_visibility(Env *, SymbolObject *, MethodVisibility);
    MethodVisibility get_method_visibility(Env *, SymbolObject *);
    MethodInfo *find_method_info(Env *, SymbolObject *);
    Method *find_method(Env *, SymbolObject *, ModuleObject ** = nullptr, Method ** = nullptr) const;
    Method *find_method(Env *, SymbolObject *, Method *) const;
    void assert_method_defined(Env *, SymbolObject *, Method *);

    Value instance_method(Env *, Value);
    Value public_instance_method(Env *, Value);

    Value instance_methods(Env *, Value, std::function<bool(MethodVisibility)>);
    Value instance_methods(Env *, Value);
    Value private_instance_methods(Env *, Value);
    Value protected_instance_methods(Env *, Value);
    Value public_instance_methods(Env *, Value);

    ArrayObject *ancestors(Env *);
    bool is_subclass_of(ModuleObject *);

    bool is_method_defined(Env *, Value) const;

    const String *inspect_str();
    Value inspect(Env *);
    Value name(Env *);
    Value attr_reader(Env *, size_t, Value *);
    Value attr_writer(Env *, size_t, Value *);
    Value attr_accessor(Env *, size_t, Value *);

    static Value attr_reader_block_fn(Env *, Value, size_t, Value *, Block *);
    static Value attr_writer_block_fn(Env *, Value, size_t, Value *, Block *);

    Value module_eval(Env *, Block *);

    Value private_method(Env *, size_t, Value *) override;
    Value protected_method(Env *, size_t, Value *) override;
    Value public_method(Env *, size_t, Value *);
    Value private_class_method(Env *, size_t, Value *);
    Value public_class_method(Env *, size_t, Value *);
    void set_method_visibility(Env *, size_t, Value *, MethodVisibility);
    Value module_function(Env *, size_t, Value *) override;

    Value deprecate_constant(Env *, size_t, Value *);
    Value private_constant(Env *, size_t, Value *);
    Value public_constant(Env *, size_t, Value *);

    bool const_defined(Env *, Value);
    Value alias_method(Env *, Value, Value);
    Value remove_method(Env *, size_t, Value *);
    Value undef_method(Env *, size_t, Value *);

    bool eqeqeq(Env *env, Value other) {
        return other->is_a(env, this);
    }

    bool has_env() { return !!m_env; }
    Env *env() { return m_env; }

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        if (m_class_name)
            snprintf(buf, len, "<ModuleObject %p name=%p>", this, m_class_name.value());
        else
            snprintf(buf, len, "<ModuleObject %p name=(none)>", this);
    }

protected:
    Env *m_env { nullptr };
    TM::Hashmap<SymbolObject *, Constant *> m_constants {};
    Optional<const String *> m_class_name {};
    ClassObject *m_superclass { nullptr };
    TM::Hashmap<SymbolObject *, Method *> m_methods {};
    TM::Hashmap<SymbolObject *, MethodInfo *> m_method_info {};
    TM::Hashmap<SymbolObject *, Value> m_class_vars {};
    Vector<ModuleObject *> m_included_modules {};
    MethodVisibility m_method_visibility { MethodVisibility::Public };
    bool m_module_function { false };
};

}
