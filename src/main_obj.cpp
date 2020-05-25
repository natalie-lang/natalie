#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *main_obj_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    return string(env, "main");
}

}
