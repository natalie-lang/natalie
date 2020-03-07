#include "natalie.h"
#include "builtin.h"

NatObject *Comparable_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_neq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) == 0) {
        return NAT_FALSE;
    } else {
        return NAT_TRUE;
    }
}

NatObject *Comparable_lt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) < 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_lte(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) <= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_gt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) > 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_gte(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = nat_send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) >= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}
