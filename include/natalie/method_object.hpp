#pragma once

#include "natalie/abstract_method_object.hpp"

namespace Natalie {

class MethodObject : public AbstractMethodObject {
public:
    MethodObject(Value object, Method *method)
        : AbstractMethodObject { Object::Type::Method, GlobalEnv::the()->Object()->const_fetch("Method"_s)->as_class(), method }
        , m_object { object } { }

    MethodObject(Value object, Method *method, SymbolObject *method_missing_name)
        : MethodObject { object, method } {
        m_method_missing_name = method_missing_name;
    }

    Value unbind(Env *);
    bool eq(Env *, Value);
    Value ltlt(Env *, Value);
    Value gtgt(Env *, Value);
    Value source_location();

    Value inspect(Env *env) {
        auto the_owner = owner();
        auto name = m_method_missing_name ? m_method_missing_name->string() : m_method->name();
        if (the_owner->is_class() && the_owner->as_class()->is_singleton())
            return StringObject::format("#<Method: {}.{}(*)>", m_object->inspect_str(env), name);
        else
            return StringObject::format("#<Method: {}#{}(*)>", owner()->inspect_str(), name);
    }

    virtual ProcObject *to_proc(Env *env) override {
        auto block = new Block { env, m_object, m_method->fn(), m_method->arity(), Block::BlockType::Method };
        return new ProcObject { block };
    }

    Value call(Env *env, Args args, Block *block) {
        if (m_method_missing_name)
            return m_object->method_missing_send(env, m_method_missing_name, args, block);
        return m_method->call(env, m_object, args, block);
    }

    virtual void visit_children(Visitor &visitor) override final {
        AbstractMethodObject::visit_children(visitor);
        visitor.visit(m_object);
        visitor.visit(m_method);
        visitor.visit(m_method_missing_name);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<MethodObject %p method=", this);
    }

private:
    Value m_object { nullptr };
    SymbolObject *m_method_missing_name { nullptr };
};
}
