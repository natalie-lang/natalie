#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Comparable_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == ValueType::Integer && NAT_INT_VALUE(result) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_neq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == ValueType::Integer && NAT_INT_VALUE(result) == 0) {
        return NAT_FALSE;
    } else {
        return NAT_TRUE;
    }
}

Value *Comparable_lt(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == ValueType::Integer && NAT_INT_VALUE(result) < 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_lte(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == ValueType::Integer && NAT_INT_VALUE(result) <= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_gt(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == ValueType::Integer && NAT_INT_VALUE(result) > 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_gte(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self, "<=>", argc, args, NULL);
    if (NAT_TYPE(result) == ValueType::Integer && NAT_INT_VALUE(result) >= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
