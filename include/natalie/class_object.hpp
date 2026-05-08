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
    static ClassObject *create() {
        return new ClassObject();
    }

    static ClassObject *create(ClassObject *klass) {
        return new ClassObject(klass);
    }

    static ClassObject *create(const ClassObject &other) {
        return new ClassObject(other);
    }

    ClassObject *superclass(Env *env) override {
        if (!m_is_initialized)
            env->raise("TypeError", "uninitialized class");
        // m_superclass may point at iclass nodes that are internal-only.
        // The user-visible superclass skips past them.
        ClassObject *s = m_superclass;
        while (s && s->is_iclass()) s = s->m_superclass;
        return s;
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

    Value initialize(Env *, Optional<Value>, Block *);

    using AllocFunc = Value (*)(Env *, Value);
    AllocFunc alloc_func() const { return m_alloc_func; }
    void set_alloc_func(AllocFunc func) { m_alloc_func = func; }

    bool is_singleton() const { return m_is_singleton; }
    void set_is_singleton(bool is_singleton) { m_is_singleton = is_singleton; }

    // For a singleton class, the object/class/module it is the singleton of.
    // Cvars assigned inside `class << foo` blocks belong on the attached object,
    // matching Ruby's lexical-scope semantics for class variables.
    Object *attached_object() const { return m_attached_object; }
    void set_attached_object(Object *obj) { m_attached_object = obj; }

    virtual void visit_children(Visitor &) const override;

    virtual String backtrace_name() const override final;

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<ClassObject {h} name=\"{}\">", this, m_name.value_or("none"));
    }

protected:
    ClassObject()
        : ClassObject { GlobalEnv::the()->Class() } { }

    ClassObject(ClassObject *klass)
        : ModuleObject { Object::Type::Class, klass } { }

    ClassObject(const ClassObject &other)
        : ModuleObject { other }
        , m_object_type { other.m_object_type }
        , m_is_singleton { other.m_is_singleton }
        , m_is_initialized { other.m_is_initialized } { }

private:
    Type m_object_type { Type::Object };
    bool m_is_singleton { false };
    bool m_is_initialized { false };
    AllocFunc m_alloc_func { nullptr };
    Object *m_attached_object { nullptr };
};

}
