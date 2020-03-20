#include "gc.h"
#include "natalie.h"
#include "builtin.h"

// TODO: move to BasicObject
NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *obj = nat_alloc(env, self, NAT_VALUE_OTHER);
    return nat_initialize(env, obj, argc, args, kwargs, block);
}
