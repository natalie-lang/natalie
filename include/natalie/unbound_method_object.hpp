#pragma once

#include "natalie/abstract_method_object.hpp"
#include "natalie/method_object.hpp"

namespace Natalie {

class UnboundMethodObject : public AbstractMethodObject {
public:
    UnboundMethodObject(ModuleObject *module_or_class, Method *method)
        : AbstractMethodObject { Object::Type::UnboundMethod, GlobalEnv::the()->Object()->const_fetch("UnboundMethod"_s).as_class(), method }
        , m_module_or_class { module_or_class } { }

    UnboundMethodObject(const UnboundMethodObject &other)
        : AbstractMethodObject { Object::Type::UnboundMethod, GlobalEnv::the()->Object()->const_fetch("UnboundMethod"_s).as_class(), other.m_method }
        , m_module_or_class { other.m_module_or_class } { }

    Value bind(Env *env, Value obj) {
        if (owner()->type() != Type::Class || obj.is_a(env, owner())) {
            return new MethodObject { obj, m_method };
        } else {
            env->raise("TypeError", "bind argument must be an instance of {}", owner()->inspect_module());
        }
    }

    Value bind_call(Env *env, Value obj, Args &&args, Block *block) {
        return bind(env, obj).as_method()->call(env, std::move(args), block);
    }

    Value bind_call(Env *env, Args &&args, Block *block) {
        args.ensure_argc_at_least(env, 1);
        auto obj = args.shift();
        return bind_call(env, obj, std::move(args), block);
    }

    bool eq(Env *env, Value other_value) {
        if (other_value.is_unbound_method()) {
            auto other = other_value.as_unbound_method();
            return m_module_or_class == other->m_module_or_class && m_method == other->m_method;
        } else {
            return false;
        }
    }

    Value inspect(Env *env) {
        if (owner() != m_module_or_class) {
            return StringObject::format("#<UnboundMethod: {}({})#{}(*)>", m_module_or_class->inspect_module(), owner()->inspect_module(), m_method->name());
        } else {
            return StringObject::format("#<UnboundMethod: {}#{}(*)>", owner()->inspect_module(), m_method->name());
        }
    }

    virtual void visit_children(Visitor &visitor) const override final {
        AbstractMethodObject::visit_children(visitor);
        visitor.visit(m_module_or_class);
        visitor.visit(m_method);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<UnboundMethodObject {h} method={h}>", this, m_method);
    }

private:
    ModuleObject *m_module_or_class { nullptr };
};

}
