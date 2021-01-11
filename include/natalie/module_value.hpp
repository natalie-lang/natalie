#pragma once

#include <assert.h>
#include <string.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

void copy_hashmap(Env *, struct hashmap &, const struct hashmap &);

struct ModuleValue : Value {
    ModuleValue(Env *);
    ModuleValue(Env *, const char *);
    ModuleValue(Env *, Type, ClassValue *);

    ModuleValue(Env *env, ClassValue *klass)
        : ModuleValue { env, Type::Module, klass } { }

    ModuleValue(Env *env, ModuleValue &other)
        : Value { other.type(), other.klass() }
        , m_class_name { GC_STRDUP(other.m_class_name) }
        , m_superclass { other.m_superclass } {
        copy_hashmap(env, m_constants, other.m_constants);
        copy_hashmap(env, m_methods, other.m_methods);
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

    virtual ValuePtr const_get(Env *, const char *) override;
    virtual ValuePtr const_fetch(Env *, const char *) override;
    virtual ValuePtr const_find(Env *, const char *, ConstLookupSearchMode = ConstLookupSearchMode::Strict, ConstLookupFailureMode = ConstLookupFailureMode::Raise) override;
    virtual ValuePtr const_set(Env *, const char *, ValuePtr) override;

    virtual void alias(Env *, const char *, const char *) override;

    ValuePtr eval_body(Env *, ValuePtr (*)(Env *, ValuePtr));

    const char *class_name() { return m_class_name; }
    void set_class_name(const char *name) { m_class_name = name ? GC_STRDUP(name) : nullptr; }

    ClassValue *superclass() { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassValue *superclass) { m_superclass = superclass; }

    ValuePtr included_modules(Env *);
    Vector<ModuleValue *> included_modules() { return m_included_modules; }
    bool does_include_module(Env *, ValuePtr);

    virtual ValuePtr cvar_get_or_null(Env *, const char *) override;
    virtual ValuePtr cvar_set(Env *, const char *, ValuePtr) override;

    ValuePtr define_method(Env *, ValuePtr, Block *);
    virtual void define_method(Env *, const char *, MethodFnPtr) override;
    virtual void define_method_with_block(Env *, const char *, Block *) override;
    virtual void undefine_method(Env *, const char *) override;

    void methods(Env *, ArrayValue *);
    Method *find_method(Env *, const char *, ModuleValue **);
    Method *find_method_without_undefined(Env *, const char *, ModuleValue **);

    ValuePtr call_method(Env *, ValuePtr, const char *, ValuePtr, size_t, ValuePtr *, Block *);

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

protected:
    hashmap m_constants {};
    const char *m_class_name { nullptr };
    ClassValue *m_superclass { nullptr };
    hashmap m_methods {};
    hashmap m_class_vars {};
    Vector<ModuleValue *> m_included_modules {};
};

}
