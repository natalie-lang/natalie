#pragma once

#include <assert.h>
#include <string.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ModuleValue : Value {

    ModuleValue(Env *);
    ModuleValue(Env *, const char *);
    ModuleValue(Env *, Type, ClassValue *);

    ModuleValue(const ModuleValue &other)
        : Value { other.type, other.klass }
        , m_class_name { strdup(other.m_class_name) }
        , m_superclass { other.m_superclass } {
        copy_hashmap(m_constants, other.m_constants);
        copy_hashmap(m_methods, other.m_methods);
        for (ModuleValue *module : const_cast<ModuleValue &>(other).m_included_modules) {
            m_included_modules.push(module);
        }
    }

    ~ModuleValue() {
        delete m_class_name;
    }

    void extend(Env *, ModuleValue *);
    void include(Env *, ModuleValue *);
    void prepend(Env *, ModuleValue *);

    virtual Value *const_get(Env *, const char *, bool) override;
    virtual Value *const_get_or_null(Env *, const char *, bool, bool) override;
    virtual Value *const_set(Env *, const char *, Value *) override;

    virtual void alias(Env *, const char *, const char *) override;

    Value *eval_body(Env *, Value *(*)(Env *, Value *));

    const char *class_name() { return m_class_name; }
    void set_class_name(const char *name) { m_class_name = name ? strdup(name) : nullptr; }

    ClassValue *superclass() { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassValue *superclass) { m_superclass = superclass; }

    Vector<ModuleValue *> included_modules() { return m_included_modules; }

    virtual Value *cvar_get_or_null(Env *, const char *) override;
    virtual Value *cvar_set(Env *, const char *, Value *) override;

    virtual void define_method(Env *, const char *, Value *(*)(Env *, Value *, ssize_t, Value **, Block *block)) override;
    virtual void define_method_with_block(Env *, const char *, Block *) override;
    virtual void undefine_method(Env *, const char *) override;

    void methods(Env *, ArrayValue *);
    Method *find_method(const char *, ModuleValue **);
    Method *find_method_without_undefined(const char *, ModuleValue **);

    Value *call_method(Env *, Value *, const char *, Value *, ssize_t, Value **, Block *);

    ArrayValue *ancestors(Env *);

    bool is_method_defined(Env *, Value *);

protected:
    struct hashmap m_constants EMPTY_HASHMAP;
    const char *m_class_name { nullptr };
    ClassValue *m_superclass { nullptr };
    struct hashmap m_methods EMPTY_HASHMAP;
    struct hashmap m_class_vars EMPTY_HASHMAP;
    Vector<ModuleValue *> m_included_modules {};
};

}
