#include "natalie.hpp"

using namespace Natalie;

/*NAT_DECLARATIONS*/

Value __FN_NAME__(Env *env, Value self, bool require_once) {
    /*NAT_EVAL_INIT*/
    auto file_name_sym = "FILE_NAME"_s;
    if (require_once && GlobalEnv::the()->has_file(file_name_sym)) {
        return Value::False();
    }
    GlobalEnv::the()->add_file(env, file_name_sym);
    /*NAT_EVAL_BODY*/
    return Value::True();
}
