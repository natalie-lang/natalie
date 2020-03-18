#include "natalie.h"
#include "builtin.h"

NatObject *Integer_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(0);
    char *str = int_to_string(NAT_INT_VALUE(self));
    return nat_string(env, str);
}

NatObject *Integer_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = NAT_INT_VALUE(self) + NAT_INT_VALUE(arg);
    return nat_integer(env, result);
}

NatObject *Integer_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = NAT_INT_VALUE(self) - NAT_INT_VALUE(arg);
    return nat_integer(env, result);
}

NatObject *Integer_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = NAT_INT_VALUE(self) * NAT_INT_VALUE(arg);
    return nat_integer(env, result);
}

NatObject *Integer_div(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");

    int64_t dividend = NAT_INT_VALUE(self);
    int64_t divisor = NAT_INT_VALUE(arg);
    if (divisor == 0) {
        NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "ZeroDivisionError"), "divided by 0");
    }
    int64_t result = dividend / divisor;
    return nat_integer(env, result);
}

NatObject *Integer_cmp(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    if (NAT_TYPE(arg) != NAT_VALUE_INTEGER) return NAT_NIL;
    int64_t i1 = NAT_INT_VALUE(self);
    int64_t i2 = NAT_INT_VALUE(arg);
    if (i1 < i2) {
        return nat_integer(env, -1);
    } else if (i1 == i2) {
        return nat_integer(env, 0);
    } else {
        return nat_integer(env, 1);
    }
}

NatObject *Integer_eqeqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_INTEGER && NAT_INT_VALUE(self) == NAT_INT_VALUE(arg)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Integer_times(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(0);
    int64_t val = NAT_INT_VALUE(self);
    assert(val >= 0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    NatObject *num;
    for (long long i=0; i<val; i++) {
        num = nat_integer(env, i);
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &num, NULL, NULL);
    }
    return self;
}

NatObject *Integer_bitwise_and(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    return nat_integer(env, NAT_INT_VALUE(self) & NAT_INT_VALUE(arg));
}

NatObject *Integer_bitwise_or(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    return nat_integer(env, NAT_INT_VALUE(self) | NAT_INT_VALUE(arg));
}

NatObject *Integer_succ(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(0);
    return nat_integer(env, NAT_INT_VALUE(self) + 1);
}
