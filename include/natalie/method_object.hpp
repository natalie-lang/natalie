#pragma once

#include "natalie/abstract_method_object.hpp"

namespace Natalie {

class MethodObject : public AbstractMethodObject {
public:
    MethodObject(Value object, Method *method)
        : AbstractMethodObject { Object::Type::Method, GlobalEnv::the()->Object()->const_fetch("Method"_s)->as_class(), method }
        , m_object { object } { }

    Value unbind(Env *);
    bool eq(Env *, Value);

    Value inspect(Env *env) {
        auto the_owner = owner();
        if (the_owner->is_class() && the_owner->as_class()->is_singleton())
            return StringObject::format(env, "#<Method: {}.{}(*)>", m_object->inspect_str(env), m_method->name());
        else
            return StringObject::format(env, "#<Method: {}#{}(*)>", owner()->inspect_str(), m_method->name());
    }

    virtual ProcObject *to_proc(Env *env) override {
        auto block = new Block { env, m_object, m_method->fn(), m_method->arity() };
        return new ProcObject { block };
    }

    Value call(Env *env, size_t argc, Value *args, Block *block) {
        return m_method->call(env, m_object, argc, args, block);
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
};
}
