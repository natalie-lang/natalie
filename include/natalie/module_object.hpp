#pragma once

#include <assert.h>
#include <string.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
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

    ValuePtr extend(Env *, size_t argc, ValuePtr *args);
    void extend_once(Env *, ModuleObject *);

    ValuePtr include(Env *, size_t argc, ValuePtr *args);
    void include_once(Env *, ModuleObject *);

    ValuePtr prepend(Env *, size_t argc, ValuePtr *args);
    void prepend_once(Env *, ModuleObject *);

    virtual ValuePtr const_find(Env *env, SymbolObject *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise) override;
    virtual ValuePtr const_get(SymbolObject *) override;
    virtual ValuePtr const_fetch(SymbolObject *) override;
    virtual ValuePtr const_set(SymbolObject *, ValuePtr) override;

    virtual void alias(Env *, SymbolObject *, SymbolObject *) override;

    ValuePtr eval_body(Env *, ValuePtr (*)(Env *, ValuePtr));

    Optional<const String *> class_name() {
        return m_class_name;
    }

    const String *class_name_or_blank() {
        if (m_class_name)
            return m_class_name.value();
        else
            return new String("");
    }

    void set_class_name(const String *name) {
        assert(name);
        m_class_name = name;
    }

    void set_class_name(const char *name) {
        assert(name);
        m_class_name = new String(name);
    }

    ClassObject *superclass() { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassObject *superclass) { m_superclass = superclass; }

    ValuePtr included_modules(Env *);
    Vector<ModuleObject *> included_modules() { return m_included_modules; }
    bool does_include_module(Env *, ValuePtr);

    virtual ValuePtr cvar_get_or_null(Env *, SymbolObject *) override;
    virtual ValuePtr cvar_set(Env *, SymbolObject *, ValuePtr) override;

    ValuePtr define_method(Env *, ValuePtr, Block *);
    virtual SymbolObject *define_method(Env *, SymbolObject *, MethodFnPtr, int) override;
    virtual SymbolObject *define_method(Env *, SymbolObject *, Block *) override;
    virtual SymbolObject *undefine_method(Env *, SymbolObject *) override;

    void methods(Env *, ArrayObject *);
    Method *find_method(Env *, SymbolObject *, ModuleObject ** = nullptr, Method * = nullptr) const;

    ArrayObject *ancestors(Env *);

    bool is_method_defined(Env *, ValuePtr);

    ValuePtr inspect(Env *);
    ValuePtr name(Env *);
    ValuePtr attr_reader(Env *, size_t, ValuePtr *);
    ValuePtr attr_writer(Env *, size_t, ValuePtr *);
    ValuePtr attr_accessor(Env *, size_t, ValuePtr *);

    static ValuePtr attr_reader_block_fn(Env *, ValuePtr, size_t, ValuePtr *, Block *);
    static ValuePtr attr_writer_block_fn(Env *, ValuePtr, size_t, ValuePtr *, Block *);

    ValuePtr module_eval(Env *, Block *);

    virtual ValuePtr private_method(Env *, ValuePtr method_name) override;
    virtual ValuePtr protected_method(Env *, ValuePtr method_name) override;
    ValuePtr public_method(Env *, ValuePtr method_name);

    bool const_defined(Env *, ValuePtr);
    ValuePtr alias_method(Env *, ValuePtr, ValuePtr);

    bool eqeqeq(Env *env, ValuePtr other) {
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
    TM::Hashmap<SymbolObject *, Object *> m_constants {};
    Optional<const String *> m_class_name {};
    ClassObject *m_superclass { nullptr };
    TM::Hashmap<SymbolObject *, Method *> m_methods {};
    TM::Hashmap<SymbolObject *, Object *> m_class_vars {};
    Vector<ModuleObject *> m_included_modules {};
    MethodVisibility m_method_visibility { MethodVisibility::Public };
};

}
