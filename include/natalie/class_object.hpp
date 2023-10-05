#pragma once

#include <assert.h>

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/module_object.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class ClassObject : public ModuleObject {
public:
    ClassObject()
        : ClassObject { GlobalEnv::the()->Class() } { }

    ClassObject(ClassObject *klass)
        : ModuleObject { Object::Type::Class, klass } { }

    ClassObject(const ClassObject &other)
        : ModuleObject { other }
        , m_object_type { other.m_object_type }
        , m_is_singleton { other.m_is_singleton }
        , m_is_initialized { other.m_is_initialized } { }

    ClassObject *superclass(Env *env) override {
        if (!m_is_initialized)
            env->raise("TypeError", "uninitialized class");
        return m_superclass;
    }

    ClassObject *subclass(Env *env, const char *name) {
        return subclass(env, name, m_object_type);
    }

    ClassObject *subclass(Env *env, const char *name, Type object_type) {
        return subclass(env, String(name), object_type);
    }

    ClassObject *subclass(Env *env, String name) {
        return subclass(env, name, m_object_type);
    }

    ClassObject *subclass(Env *, String, Type);

    void initialize_subclass(ClassObject *, Env *, String, Optional<Type> = {});
    void initialize_subclass_without_checks(ClassObject *, Env *, String, Optional<Type> = {});

    static ClassObject *bootstrap_class_class(Env *);
    static ClassObject *bootstrap_basic_object(Env *, ClassObject *);

    Type object_type() { return m_object_type; }

    Value initialize(Env *, Value, Block *);

    bool is_singleton() const { return m_is_singleton; }
    void set_is_singleton(bool is_singleton) { m_is_singleton = is_singleton; }

    virtual String backtrace_name() const override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        if (m_class_name)
            snprintf(buf, len, "<ClassObject %p name=%s>", this, m_class_name.value().c_str());
        else
            snprintf(buf, len, "<ClassObject %p name=(none)>", this);
    }

private:
    Type m_object_type { Type::Object };
    bool m_is_singleton { false };
    bool m_is_initialized { false };
};

}
