#include "natalie.h"
#include "builtin.h"

// TODO: move to BasicObject
NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    return nat_new(env, self, argc, args, kwargs, block);
}
