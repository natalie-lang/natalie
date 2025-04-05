#pragma once

#include "natalie/abstract_method_object.hpp"

namespace Natalie {

class MethodObject : public AbstractMethodObject {
public:
    MethodObject(Value object, Method *method)
        : AbstractMethodObject { Object::Type::Method, GlobalEnv::the()->Object()->const_fetch("Method"_s).as_class(), method }
        , m_object { object } { }

    MethodObject(Value object, Method *method, SymbolObject *method_missing_name)
        : MethodObject { object, method } {
        m_method_missing_name = method_missing_name;
    }

    Value unbind(Env *);
    bool eq(Env *, Value);
    Value ltlt(Env *, Value);
    Value gtgt(Env *, Value);
    Value hash() const;
    Value source_location();

    Value inspect(Env *env) {
        auto the_owner = owner();
        auto name = m_method_missing_name ? m_method_missing_name->string() : m_method->name();
        if (the_owner->type() == Type::Class && static_cast<ClassObject *>(the_owner)->is_singleton())
            return StringObject::format("#<Method: {}.{}(*)>", m_object.inspected(env), name);
        else
            return StringObject::format("#<Method: {}#{}(*)>", owner()->inspect_string(), name);
    }

    virtual ProcObject *to_proc(Env *env) override {
        auto block = new Block { *env, m_object, m_method->fn(), m_method->arity(), Block::BlockType::Method };
        return new ProcObject { block };
    }

    Value call(Env *env, Args &&args, Block *block) {
        if (m_method_missing_name)
            return m_object->method_missing_send(env, m_method_missing_name, std::move(args), block);
        return m_method->call(env, m_object, std::move(args), block);
    }

    virtual void visit_children(Visitor &visitor) const override final {
        AbstractMethodObject::visit_children(visitor);
        visitor.visit(m_object);
        visitor.visit(m_method);
        visitor.visit(m_method_missing_name);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<MethodObject {h} method={}>", this, m_object.dbg_inspect(indent));
    }

private:
    Value m_object;
    SymbolObject *m_method_missing_name { nullptr };
};
}
