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

    ModuleValue(Env *env, ClassValue *klass)
        : ModuleValue { env, Type::Module, klass } { }

    ModuleValue(ModuleValue &other)
        : Value { other.type(), other.klass() }
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

    Value *extend(Env *, ssize_t argc, Value **args);
    void extend_once(Env *, ModuleValue *);

    Value *include(Env *, ssize_t argc, Value **args);
    void include_once(Env *, ModuleValue *);

    Value *prepend(Env *, ssize_t argc, Value **args);
    void prepend_once(Env *, ModuleValue *);

    virtual Value *const_get(Env *, const char *, bool) override;
    virtual Value *const_get_or_panic(Env *, const char *, bool) override;
    virtual Value *const_get_or_null(Env *, const char *, bool, bool) override;
    virtual Value *const_set(Env *, const char *, Value *) override;

    virtual void alias(Env *, const char *, const char *) override;

    Value *eval_body(Env *, Value *(*)(Env *, Value *));

    const char *class_name() { return m_class_name; }
    void set_class_name(const char *name) { m_class_name = name ? strdup(name) : nullptr; }

    ClassValue *superclass() { return m_superclass; }
    void set_superclass_DANGEROUSLY(ClassValue *superclass) { m_superclass = superclass; }

    Value *included_modules(Env *);
    Vector<ModuleValue *> included_modules() { return m_included_modules; }
    bool does_include_module(Env *, Value *);

    virtual Value *cvar_get_or_null(Env *, const char *) override;
    virtual Value *cvar_set(Env *, const char *, Value *) override;

    Value *define_method(Env *, Value *, Block *);
    virtual void define_method(Env *, const char *, Value *(*)(Env *, Value *, ssize_t, Value **, Block *block)) override;
    virtual void define_method_with_block(Env *, const char *, Block *) override;
    virtual void undefine_method(Env *, const char *) override;

    void methods(Env *, ArrayValue *);
    Method *find_method(Env *env, const char *, ModuleValue **);
    Method *find_method_without_undefined(Env *env, const char *, ModuleValue **);

    Value *call_method(Env *, Value *, const char *, Value *, ssize_t, Value **, Block *);

    ArrayValue *ancestors(Env *);

    bool is_method_defined(Env *, Value *);

    Value *inspect(Env *);
    Value *name(Env *);
    Value *attr_reader(Env *, ssize_t, Value **);
    Value *attr_writer(Env *, ssize_t, Value **);
    Value *attr_accessor(Env *, ssize_t, Value **);

    static Value *attr_reader_block_fn(Env *, Value *, ssize_t, Value **, Block *);
    static Value *attr_writer_block_fn(Env *, Value *, ssize_t, Value **, Block *);

    Value *class_eval(Env *, Block *);

    Value *private_method(Env *, Value *method_name);
    Value *protected_method(Env *, Value *method_name);
    Value *public_method(Env *, Value *method_name);

    bool const_defined(Env *, Value *);
    Value *alias_method(Env *, Value *, Value *);

    bool eqeqeq(Env *env, Value *other) {
        return other->is_a(env, this);
    }

protected:
    struct hashmap m_constants EMPTY_HASHMAP;
    const char *m_class_name { nullptr };
    ClassValue *m_superclass { nullptr };
    struct hashmap m_methods EMPTY_HASHMAP;
    struct hashmap m_class_vars EMPTY_HASHMAP;
    Vector<ModuleValue *> m_included_modules {};
};

}
