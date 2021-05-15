#pragma once

#include <assert.h>
#include <string.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/hashmap.hpp"
#include "natalie/macros.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/value.hpp"
#include "tm/optional.hpp"

namespace Natalie {

using namespace TM;

class ModuleValue : public Value {
public:
    ModuleValue(Env *);
    ModuleValue(Env *, const char *);
    ModuleValue(Env *, Type, ClassValue *);

    ModuleValue(Env *env, ClassValue *klass)
        : ModuleValue { env, Type::Module, klass } { }

    ModuleValue(Env *env, ModuleValue &other)
        : Value { other.type(), other.klass() }
        , m_constants { other.m_constants }
        , m_class_name { other.m_class_name }
        , m_superclass { other.m_superclass }
        , m_methods { other.m_methods } {
        for (ModuleValue *module : const_cast<ModuleValue &>(other).m_included_modules) {
            m_included_modules.push(module);
        }
    }

    ValuePtr extend(Env *, size_t argc, ValuePtr *args);
    void extend_once(Env *, ModuleValue *);

    ValuePtr include(Env *, size_t argc, ValuePtr *args);
    void include_once(Env *, ModuleValue *);

    ValuePtr prepend(Env *, size_t argc, ValuePtr *args);
    void prepend_once(Env *, ModuleValue *);

    virtual ValuePtr const_get(Env *, SymbolValue *) override;
    virtual ValuePtr const_fetch(Env *, SymbolValue *) override;
    virtual ValuePtr const_find(Env *, SymbolValue *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise) override;
    virtual ValuePtr const_set(Env *, SymbolValue *, ValuePtr) override;

    virtual void alias(Env *, SymbolValue *, SymbolValue *) override;

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

    ClassValue *superclass() { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassValue *superclass) { m_superclass = superclass; }

    ValuePtr included_modules(Env *);
    Vector<ModuleValue *> included_modules() { return m_included_modules; }
    bool does_include_module(Env *, ValuePtr);

    virtual ValuePtr cvar_get_or_null(Env *, SymbolValue *) override;
    virtual ValuePtr cvar_set(Env *, SymbolValue *, ValuePtr) override;

    ValuePtr define_method(Env *, ValuePtr, Block *);
    virtual SymbolValue *define_method(Env *, SymbolValue *, MethodFnPtr, int) override;
    virtual SymbolValue *define_method(Env *, SymbolValue *, Block *) override;
    virtual SymbolValue *undefine_method(Env *, SymbolValue *) override;

    void methods(Env *, ArrayValue *);
    Method *find_method(Env *, SymbolValue *, ModuleValue ** = nullptr, Method * = nullptr);

    ArrayValue *ancestors(Env *);

    bool is_method_defined(Env *, ValuePtr);

    ValuePtr inspect(Env *);
    ValuePtr name(Env *);
    ValuePtr attr_reader(Env *, size_t, ValuePtr *);
    ValuePtr attr_writer(Env *, size_t, ValuePtr *);
    ValuePtr attr_accessor(Env *, size_t, ValuePtr *);

    static ValuePtr attr_reader_block_fn(Env *, ValuePtr, size_t, ValuePtr *, Block *);
    static ValuePtr attr_writer_block_fn(Env *, ValuePtr, size_t, ValuePtr *, Block *);

    ValuePtr module_eval(Env *, Block *);

    ValuePtr private_method(Env *, ValuePtr method_name);
    ValuePtr protected_method(Env *, ValuePtr method_name);
    ValuePtr public_method(Env *, ValuePtr method_name);

    bool const_defined(Env *, ValuePtr);
    ValuePtr alias_method(Env *, ValuePtr, ValuePtr);

    bool eqeqeq(Env *env, ValuePtr other) {
        return other->is_a(env, this);
    }

    Env *env() { return &m_env; }

    virtual void visit_children(Visitor &) override final;

    virtual void gc_print() override {
        fprintf(stderr, "<ModuleValue %p name=%s>", this, m_class_name ? m_class_name.value()->c_str() : "null");
    }

protected:
    Env m_env {};
    Hashmap<SymbolValue *, Value *> m_constants {};
    Optional<const String *> m_class_name {};
    ClassValue *m_superclass { nullptr };
    Hashmap<SymbolValue *, Method *> m_methods {};
    Hashmap<SymbolValue *, Value *> m_class_vars {};
    Vector<ModuleValue *> m_included_modules {};
    MethodVisibility m_method_visibility { MethodVisibility::Public };
};

}
