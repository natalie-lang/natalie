#include "natalie.h"
#include "nat_basic_object.h"

NatObject *BasicObject_not(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    if (nat_truthy(self)) {
        return env_get(env, "false");
    } else {
        return env_get(env, "true");
    }
}
