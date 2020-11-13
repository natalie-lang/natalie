#include "natalie.hpp"

namespace Natalie {

void ExceptionValue::build_backtrace(Env *env) {
    m_backtrace = new ArrayValue { env };
    Env *bt_env = env;
    do {
        if (bt_env->file()) {
            char *method_name = env->build_code_location_name(bt_env);
            m_backtrace->push(StringValue::sprintf(env, "%s:%d:in `%s'", bt_env->file(), bt_env->line(), method_name));
            GC_FREE(method_name);
        }
        bt_env = bt_env->caller();
    } while (bt_env);
}

Value *ExceptionValue::initialize(Env *env, Value *message) {
    if (!message) {
        set_message(m_klass->class_name());
    } else {
        if (!message->is_string()) {
            message = message->send(env, "inspect");
        }
        set_message(message->as_string()->c_str());
    }
    return this;
}

Value *ExceptionValue::inspect(Env *env) {
    return StringValue::sprintf(env, "#<%S: %s>", m_klass->send(env, "inspect")->as_string(), m_message);
}

Value *ExceptionValue::message(Env *env) {
    return new StringValue { env, m_message };
}

Value *ExceptionValue::backtrace(Env *env) {
    return m_backtrace ? m_backtrace->dup(env) : env->nil_obj();
}

}
