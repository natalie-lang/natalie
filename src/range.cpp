#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Range_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2, 3);
    bool exclude_end = false;
    if (argc == 3) {
        exclude_end = truthy(args[2]);
    }
    return range(env, args[0], args[1], exclude_end);
}

Value *Range_begin(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Range);
    return self->range_begin;
}

Value *Range_end(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Range);
    return self->range_end;
}

Value *Range_exclude_end(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Range);
    return self->range_exclude_end ? NAT_TRUE : NAT_FALSE;
}

Value *Range_to_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Range);
    Value *ary = array_new(env);
    Value *item = self->range_begin;
    const char *op = self->range_exclude_end ? "<" : "<=";
    while (truthy(send(env, item, op, 1, &self->range_end, NULL))) {
        array_push(env, ary, item);
        item = send(env, item, "succ", 0, NULL, NULL);
    }
    return ary;
}

Value *Range_each(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Range);
    Value *item = self->range_begin;
    const char *op = self->range_exclude_end ? "<" : "<=";
    while (truthy(send(env, item, op, 1, &self->range_end, NULL))) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL);
        item = send(env, item, "succ", 0, NULL, NULL);
    }
    return self;
}

Value *Range_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Range);
    if (self->range_exclude_end) {
        return sprintf(env, "%v...%v", self->range_begin, self->range_end);
    } else {
        return sprintf(env, "%v..%v", self->range_begin, self->range_end);
    }
}

Value *Range_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::Range);
    Value *arg = args[0];
    if (NAT_TYPE(arg) == ValueType::Range && truthy(send(env, self->range_begin, "==", 1, &arg->range_begin, NULL)) && truthy(send(env, self->range_end, "==", 1, &arg->range_end, NULL)) && self->range_exclude_end == arg->range_exclude_end) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Range_eqeqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::Range);
    Value *arg = args[0];
    if (NAT_TYPE(self->range_begin) == ValueType::Integer && NAT_TYPE(arg) == ValueType::Integer) {
        // optimized path for integer ranges
        if (NAT_INT_VALUE(self->range_begin) <= NAT_INT_VALUE(arg) && ((self->range_exclude_end && NAT_INT_VALUE(arg) < NAT_INT_VALUE(self->range_end)) || (!self->range_exclude_end && NAT_INT_VALUE(arg) <= NAT_INT_VALUE(self->range_end)))) {
            return NAT_TRUE;
        }
    } else {
        // slower method that should work for any type of range
        Value *item = self->range_begin;
        const char *op = self->range_exclude_end ? "<" : "<=";
        while (truthy(send(env, item, op, 1, &self->range_end, NULL))) {
            if (truthy(send(env, item, "==", 1, &arg, NULL))) {
                return NAT_TRUE;
            }
            item = send(env, item, "succ", 0, NULL, NULL);
        }
    }
    return NAT_FALSE;
}

}
