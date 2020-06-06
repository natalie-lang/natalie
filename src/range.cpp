#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Range_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2, 3);
    bool exclude_end = false;
    if (argc == 3) {
        exclude_end = args[2]->is_truthy();
    }
    return range_new(env, args[0], args[1], exclude_end);
}

Value *Range_begin(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    return self->range_begin;
}

Value *Range_end(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    return self->range_end;
}

Value *Range_exclude_end(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    if (self->range_exclude_end) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Range_to_a(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    Value *ary = array_new(env);
    Value *item = self->range_begin;
    const char *op = self->range_exclude_end ? "<" : "<=";
    while (send(env, item, op, 1, &self->range_end, nullptr)->is_truthy()) {
        array_push(env, ary, item);
        item = send(env, item, "succ", 0, NULL, NULL);
    }
    return ary;
}

Value *Range_each(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    Value *item = self->range_begin;
    const char *op = self->range_exclude_end ? "<" : "<=";
    while (send(env, item, op, 1, &self->range_end, nullptr)->is_truthy()) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL);
        item = send(env, item, "succ", 0, NULL, NULL);
    }
    return self;
}

Value *Range_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    if (self->range_exclude_end) {
        return sprintf(env, "%v...%v", self->range_begin, self->range_end);
    } else {
        return sprintf(env, "%v..%v", self->range_begin, self->range_end);
    }
}

Value *Range_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    RangeValue *self = self_value->as_range();
    if (NAT_TYPE(args[0]) == Value::Type::Range) {
        RangeValue *arg = args[0]->as_range();
        bool begin_equal = send(env, self->range_begin, "==", 1, &arg->range_begin, nullptr)->is_truthy();
        bool end_equal = send(env, self->range_end, "==", 1, &arg->range_end, nullptr)->is_truthy();
        if (begin_equal && end_equal && self->range_exclude_end == arg->range_exclude_end) {
            return NAT_TRUE;
        }
    }
    return NAT_FALSE;
}

Value *Range_eqeqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    RangeValue *self = self_value->as_range();
    Value *arg = args[0];
    if (NAT_TYPE(self->range_begin) == Value::Type::Integer && NAT_TYPE(arg) == Value::Type::Integer) {
        // optimized path for integer ranges
        if (NAT_INT_VALUE(self->range_begin) <= NAT_INT_VALUE(arg) && ((self->range_exclude_end && NAT_INT_VALUE(arg) < NAT_INT_VALUE(self->range_end)) || (!self->range_exclude_end && NAT_INT_VALUE(arg) <= NAT_INT_VALUE(self->range_end)))) {
            return NAT_TRUE;
        }
    } else {
        // slower method that should work for any type of range
        Value *item = self->range_begin;
        const char *op = self->range_exclude_end ? "<" : "<=";
        while (send(env, item, op, 1, &self->range_end, nullptr)->is_truthy()) {
            if (send(env, item, "==", 1, &arg, nullptr)->is_truthy()) {
                return NAT_TRUE;
            }
            item = send(env, item, "succ", 0, NULL, NULL);
        }
    }
    return NAT_FALSE;
}

}
