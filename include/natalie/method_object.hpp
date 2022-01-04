#pragma once

#include "natalie/class_object.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/method.hpp"
#include "natalie/object.hpp"
#include "natalie/proc_object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class MethodObject : public Object {
public:
    MethodObject(Value object, Method *method)
        : Object { Object::Type::Method, GlobalEnv::the()->Object()->const_fetch("Method"_s)->as_class() }
        , m_object { object }
        , m_method { method } { }

    MethodObject(ClassObject *klass)
        : Object { Object::Type::Method, klass } { }

    ModuleObject *owner() { return m_method->owner(); }
    SymbolObject *name(Env *env) { return SymbolObject::intern(m_method->name()); }
    SymbolObject *original_name(Env *env) { return SymbolObject::intern(m_method->original_name()); }
    Method *method() { return m_method; }

    Value inspect(Env *env);

    int arity() { return m_method ? m_method->arity() : 0; }

    virtual ProcObject *to_proc(Env *env) override {
        auto block = new Block { env, m_object, m_method->fn(), m_method->arity() };
        return new ProcObject { block };
    }

    virtual void visit_children(Visitor &visitor) override final {
        Object::visit_children(visitor);
        visitor.visit(m_object);
        visitor.visit(m_method);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<MethodObject %p method=", this);
    }

private:
    Value m_object { nullptr };
    Method *m_method { nullptr };
};
}
