#include "natalie.hpp"

namespace Natalie {

void ExceptionValue::build_backtrace(Env *env) {
    m_backtrace = new ArrayValue { env };
    Env *bt_env = env;
    do {
        if (bt_env->file()) {
            char *method_name = env->build_code_location_name(bt_env);
            m_backtrace->push(StringValue::sprintf(env, "%s:%d:in `%s'", bt_env->file(), bt_env->line(), method_name));
            free(method_name);
        }
        bt_env = bt_env->caller();
    } while (bt_env);
}

}
