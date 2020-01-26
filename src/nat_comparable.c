#include "natalie.h"
#include "nat_kernel.h"
#include "nat_class.h"

// FIXME: this whole module can be written in Natalie rather than in C.

NatObject *Comparable_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) == 0) {
        return true_obj;
    } else {
        return false_obj;
    }
}

NatObject *Comparable_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) == 0) {
        return false_obj;
    } else {
        return true_obj;
    }
}

NatObject *Comparable_lt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) < 0) {
        return true_obj;
    } else {
        return false_obj;
    }
}

NatObject *Comparable_gt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) > 0) {
        return true_obj;
    } else {
        return false_obj;
    }
}
