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

class AbstractMethodObject : public Object {
public:
    ModuleObject *owner() { return m_method->owner(); }
    SymbolObject *name(Env *env) { return SymbolObject::intern(m_method->name()); }
    Method *method() { return m_method; }
    int arity() { return m_method->arity(); }

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_method);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<AbstractMethodObject %p>", this);
    }

protected:
    Method *m_method { nullptr };

    AbstractMethodObject(Object::Type type, ClassObject *klass, Method *method)
        : Object { type, klass }
        , m_method { method } { }
};

}
