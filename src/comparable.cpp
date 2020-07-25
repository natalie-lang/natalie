#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Comparable_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = self_value->send(env, "<=>", argc, args, nullptr);
    if (result->is_integer() && result->as_integer()->is_zero()) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

Value *Comparable_neq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = self_value->send(env, "<=>", argc, args, nullptr);
    if (result->is_integer() && result->as_integer()->is_zero()) {
        return env->false_obj();
    } else {
        return env->true_obj();
    }
}

Value *Comparable_lt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = self_value->send(env, "<=>", argc, args, nullptr);
    if (result->is_integer() && result->as_integer()->to_int64_t() < 0) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

Value *Comparable_lte(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = self_value->send(env, "<=>", argc, args, nullptr);
    if (result->is_integer() && result->as_integer()->to_int64_t() <= 0) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

Value *Comparable_gt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = self_value->send(env, "<=>", argc, args, nullptr);
    if (result->is_integer() && result->as_integer()->to_int64_t() > 0) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

Value *Comparable_gte(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *result = self_value->send(env, "<=>", argc, args, nullptr);
    if (result->is_integer() && result->as_integer()->to_int64_t() >= 0) {
        return env->true_obj();
    } else {
        return env->false_obj();
    }
}

}
