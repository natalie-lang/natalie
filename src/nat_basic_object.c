#include "natalie.h"
#include "nat_basic_object.h"

NatObject *BasicObject_not(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    return nat_not(env, self);
}

NatObject *BasicObject_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (self == arg) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}

NatObject *BasicObject_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    return BasicObject_not(env, nat_send(env, self, "==", 1, &arg, NULL), 0, NULL, NULL, NULL);
}
