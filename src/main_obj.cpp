#include "builtin.hpp"
#include "natalie.hpp"

NatObject *main_obj_inspect(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    return nat_string(env, "main");
}
