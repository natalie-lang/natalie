#pragma once

#include "natalie/abstract_method_object.hpp"

namespace Natalie {

class UnboundMethodObject : public AbstractMethodObject {
public:
    UnboundMethodObject(ModuleObject *module_or_class, Method *method)
        : AbstractMethodObject { Object::Type::UnboundMethod, GlobalEnv::the()->Object()->const_fetch("UnboundMethod"_s)->as_class(), method }
        , m_module_or_class { module_or_class } { }

    Value bind(Env *env, Value obj) {
        if (!owner()->is_class() || obj->is_a(env, owner())) {
            return new MethodObject { obj, m_method };
        } else {
            env->raise("TypeError", "bind argument must be an instance of {}", owner()->inspect_str());
        }
    }

    bool eq(Env *env, Value other_value) {
        if (other_value->is_unbound_method()) {
            auto other = other_value->as_unbound_method();
            return m_module_or_class == other->m_module_or_class && m_method == other->m_method;
        } else {
            return false;
        }
    }

    Value inspect(Env *env) {
        if (owner() != m_module_or_class) {
            return StringObject::format(env, "#<UnboundMethod: {}({})#{}(*)>", m_module_or_class->inspect_str(), owner()->inspect_str(), m_method->name());
        } else {
            return StringObject::format(env, "#<UnboundMethod: {}#{}(*)>", owner()->inspect_str(), m_method->name());
        }
    }

    virtual void visit_children(Visitor &visitor) override final {
        Object::visit_children(visitor);
        visitor.visit(m_module_or_class);
        visitor.visit(m_method);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<UnboundMethodObject %p method=%p>", this, m_method);
    }

private:
    ModuleObject *m_module_or_class { nullptr };
};

}
