#include "natalie.h"
#include "nat_object.h"

// TODO: move to BasicObject
NatObject *Object_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    return nat_new(env, self, argc, args, kwargs, block);
}
