#include "natalie.h"
#include "nat_kernel.h"
#include "nat_class.h"

// FIXME: this whole module can be written in Natalie rather than in C.

NatObject *Comparable_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (result->type == NAT_VALUE_INTEGER && result->integer == 0) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}

NatObject *Comparable_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (result->type == NAT_VALUE_INTEGER && result->integer == 0) {
        return env_get(env, "false");
    } else {
        return env_get(env, "true");
    }
}

NatObject *Comparable_lt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (result->type == NAT_VALUE_INTEGER && result->integer < 0) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}

NatObject *Comparable_gt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (result->type == NAT_VALUE_INTEGER && result->integer > 0) {
        return env_get(env, "true");
    } else {
        return env_get(env, "false");
    }
}
