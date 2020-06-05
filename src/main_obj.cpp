#include "natalie/builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *main_obj_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    return string(env, "main");
}

}
