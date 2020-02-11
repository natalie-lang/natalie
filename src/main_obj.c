#include "natalie.h"
#include "builtin.h"

NatObject *main_obj_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    return nat_string(env, "main");
}
