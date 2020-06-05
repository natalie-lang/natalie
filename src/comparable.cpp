#include "natalie/builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Comparable_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self_value, "<=>", argc, args, NULL);
    if (result->is_integer() && result->as_integer()->is_zero()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_neq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self_value, "<=>", argc, args, NULL);
    if (result->is_integer() && result->as_integer()->is_zero()) {
        return NAT_FALSE;
    } else {
        return NAT_TRUE;
    }
}

Value *Comparable_lt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self_value, "<=>", argc, args, NULL);
    if (result->is_integer() && result->as_integer()->to_int64_t() < 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_lte(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self_value, "<=>", argc, args, NULL);
    if (result->is_integer() && result->as_integer()->to_int64_t() <= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_gt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self_value, "<=>", argc, args, NULL);
    if (result->is_integer() && result->as_integer()->to_int64_t() > 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Comparable_gte(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = send(env, self_value, "<=>", argc, args, NULL);
    if (result->is_integer() && result->as_integer()->to_int64_t() >= 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
