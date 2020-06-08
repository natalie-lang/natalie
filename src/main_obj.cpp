#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *main_obj_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    return new StringValue { env, "main" };
}

}
