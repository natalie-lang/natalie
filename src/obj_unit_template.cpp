#include "natalie.hpp"

using namespace Natalie;

/*NAT_DECLARATIONS*/

Value init_OBJ_NAME(Env *env, Value self) {
    Env file_env { *env };
    file_env.set_caller(env->caller());
    file_env.build_vars(0);
    env = &file_env;
    /*NAT_OBJ_INIT*/
    /*NAT_EVAL_INIT*/
    try {
        return [&]() -> Value {
            /*NAT_EVAL_BODY*/
            return Value::nil();
        }();
    } catch (ExceptionObject *exception) {
        if (exception->local_jump_error_type() == LocalJumpErrorType::Return
            && exception->is_local_jump_error_with_break_point(-1)) {
            return exception->ivar_get(env, "@exit_value"_s);
        }
        throw;
    }
}
