#include "natalie.h"
#include "nat_integer.h"

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
    assert(NAT_TYPE(arg) == NAT_VALUE_INTEGER);
    int64_t result = NAT_INT_VALUE(self) + NAT_INT_VALUE(arg);
    return nat_integer(env, result);
}

NatObject *Integer_sub(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    assert(NAT_TYPE(arg) == NAT_VALUE_INTEGER);
    int64_t result = NAT_INT_VALUE(self) - NAT_INT_VALUE(arg);
    return nat_integer(env, result);
}

NatObject *Integer_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    assert(NAT_TYPE(arg) == NAT_VALUE_INTEGER);
    int64_t result = NAT_INT_VALUE(self) * NAT_INT_VALUE(arg);
    return nat_integer(env, result);
}

NatObject *Integer_div(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    assert(NAT_TYPE(arg) == NAT_VALUE_INTEGER);
    // FIXME: raise ZeroDivisionError if arg is zero
    int64_t result = NAT_INT_VALUE(self) / NAT_INT_VALUE(arg);
    return nat_integer(env, result);
}

NatObject *Integer_cmp(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject* arg = args[0];
    if (NAT_TYPE(arg) != NAT_VALUE_INTEGER) return nil;
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
        return true_obj;
    } else {
        return false_obj;
    }
}

NatObject *Integer_times(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(0);
    int64_t val = NAT_INT_VALUE(self);
    assert(val >= 0);
    assert(block);
    NatObject *num;
    for (size_t i=0; i<val; i++) {
        num = nat_integer(env, i);
        nat_run_block(env, block, 1, &num, NULL, NULL);
    }
    return self;
}
