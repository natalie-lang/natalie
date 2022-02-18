#include "natalie.hpp"

namespace Natalie {

Value ExceptionObject::initialize(Env *env, Value message) {
    if (!message) {
        auto name = m_klass->inspect_str();
        set_message(new StringObject { *name });
    } else {
        if (!message->is_string()) {
            message = message.send(env, "inspect"_s);
        }
        set_message(message->as_string());
    }
    return this;
}

Value ExceptionObject::inspect(Env *env) {
    return StringObject::format("#<{}: {}>", m_klass->inspect_str(), m_message);
}

Value ExceptionObject::backtrace(Env *env) {
    return m_backtrace ? m_backtrace->dup(env) : NilObject::the();
}

Value ExceptionObject::match_rescue_array(Env *env, Value ary) {
    for (auto klass : *ary->as_array()) {
        if (klass->send(env, "==="_s, { this })->is_truthy())
            return TrueObject::the();
    }
    return FalseObject::the();
}

void ExceptionObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_message);
    visitor.visit(m_backtrace);
}

}
