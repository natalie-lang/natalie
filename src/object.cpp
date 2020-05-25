#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"

NatObject *Object_new(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    NatObject *obj = nat_alloc(env, self, NAT_VALUE_OTHER);
    return nat_initialize(env, obj, argc, args, block);
}
