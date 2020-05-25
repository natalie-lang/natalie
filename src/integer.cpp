#include "builtin.hpp"
#include "natalie.hpp"
#include <math.h>

namespace Natalie {

NatObject *Integer_to_s(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(0);
    char buf[NAT_INT_64_MAX_BUF_LEN];
    int_to_string(NAT_INT_VALUE(self), buf);
    return string(env, buf);
}

NatObject *Integer_add(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = NAT_INT_VALUE(self) + NAT_INT_VALUE(arg);
    return integer(env, result);
}

NatObject *Integer_sub(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = NAT_INT_VALUE(self) - NAT_INT_VALUE(arg);
    return integer(env, result);
}

NatObject *Integer_mul(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = NAT_INT_VALUE(self) * NAT_INT_VALUE(arg);
    return integer(env, result);
}

NatObject *Integer_div(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");

    int64_t dividend = NAT_INT_VALUE(self);
    int64_t divisor = NAT_INT_VALUE(arg);
    if (divisor == 0) {
        NAT_RAISE(env, "ZeroDivisionError", "divided by 0");
    }
    int64_t result = dividend / divisor;
    return integer(env, result);
}

NatObject *Integer_mod(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = NAT_INT_VALUE(self) % NAT_INT_VALUE(arg);
    return integer(env, result);
}

NatObject *Integer_pow(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    int64_t result = pow(NAT_INT_VALUE(self), NAT_INT_VALUE(arg));
    return integer(env, result);
}

NatObject *Integer_cmp(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) != NAT_VALUE_INTEGER) return NAT_NIL;
    int64_t i1 = NAT_INT_VALUE(self);
    int64_t i2 = NAT_INT_VALUE(arg);
    if (i1 < i2) {
        return integer(env, -1);
    } else if (i1 == i2) {
        return integer(env, 0);
    } else {
        return integer(env, 1);
    }
}

NatObject *Integer_eqeqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_INTEGER && NAT_INT_VALUE(self) == NAT_INT_VALUE(arg)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Integer_times(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(0);
    int64_t val = NAT_INT_VALUE(self);
    assert(val >= 0);
    NAT_ASSERT_BLOCK(); // TODO: return Enumerator when no block given
    NatObject *num;
    for (long long i = 0; i < val; i++) {
        num = integer(env, i);
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &num, NULL);
    }
    return self;
}

NatObject *Integer_bitwise_and(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    return integer(env, NAT_INT_VALUE(self) & NAT_INT_VALUE(arg));
}

NatObject *Integer_bitwise_or(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    return integer(env, NAT_INT_VALUE(self) | NAT_INT_VALUE(arg));
}

NatObject *Integer_succ(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_INTEGER);
    NAT_ASSERT_ARGC(0);
    return integer(env, NAT_INT_VALUE(self) + 1);
}

}
