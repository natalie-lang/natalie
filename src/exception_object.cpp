#include "natalie.hpp"

namespace Natalie {

void ExceptionObject::build_backtrace(Env *env) {
    m_backtrace = new ArrayObject {};
    Env *bt_env = env;
    do {
        if (bt_env->file()) {
            auto method_name = env->build_code_location_name(bt_env);
            m_backtrace->push(StringObject::format(env, "{}:{}:in `{}'", bt_env->file(), bt_env->line(), method_name));
        }
        bt_env = bt_env->caller();
    } while (bt_env);
}

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
    return StringObject::format(env, "#<{}: {}>", m_klass->inspect_str(), m_message);
}

Value ExceptionObject::backtrace(Env *env) {
    return m_backtrace ? m_backtrace->dup(env) : NilObject::the();
}

void ExceptionObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_message);
    visitor.visit(m_backtrace);
}

}
