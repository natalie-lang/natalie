#include "natalie.hpp"

using namespace Natalie;

/*NAT_DECLARATIONS*/

Value __FN_NAME__(Env *env, Value self, bool require_once) {
    Env file_env { *env };
    file_env.set_caller(env->caller());
    file_env.build_vars(0);
    env = &file_env;
    /*NAT_EVAL_INIT*/
    auto file_name_sym = "FILE_NAME"_s;
    if (require_once && GlobalEnv::the()->has_file(file_name_sym)) {
        return Value::False();
    }
    if (require_once)
        GlobalEnv::the()->add_file(env, file_name_sym);
    try {
        // A Ruby return exits this file's body; require still returns true.
        [&]() -> Value {
            /*NAT_EVAL_BODY*/
            return Value::True();
        }();
    } catch (ExceptionObject *exception) {
        if (exception->local_jump_error_type() != LocalJumpErrorType::Return
            || !exception->is_local_jump_error_with_break_point(-1)) {
            if (require_once)
                GlobalEnv::the()->remove_file(env, file_name_sym);
            throw;
        }
    } catch (...) {
        if (require_once)
            GlobalEnv::the()->remove_file(env, file_name_sym);
        throw;
    }
    return Value::True();
}
