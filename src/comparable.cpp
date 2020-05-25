#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Comparable_eqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_neq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) == 0) {
        return NAT_FALSE;
    } else {
        return NAT_TRUE;
    }
}

NatObject *Comparable_lt(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) < 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_lte(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) <= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_gt(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) > 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Comparable_gte(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NatObject *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == NAT_VALUE_INTEGER && NAT_INT_VALUE(result) >= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
