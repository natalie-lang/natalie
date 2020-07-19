#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Range_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2, 3);
    bool exclude_end = false;
    if (argc == 3) {
        exclude_end = args[2]->is_truthy();
    }
    return new RangeValue { env, args[0], args[1], exclude_end };
}

Value *Range_begin(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    return self->begin();
}

Value *Range_end(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    return self->end();
}

Value *Range_exclude_end(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    if (self->exclude_end()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Range_to_a(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    ArrayValue *ary = new ArrayValue { env };
    Value *item = self->begin();
    const char *op = self->exclude_end() ? "<" : "<=";
    Value *end = self->end();
    while (item->send(env, op, 1, &end, nullptr)->is_truthy()) {
        ary->push(item);
        item = item->send(env, "succ");
    }
    return ary;
}

Value *Range_each(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    Value *item = self->begin();
    const char *op = self->exclude_end() ? "<" : "<=";
    Value *end = self->end();
    while (item->send(env, op, 1, &end, nullptr)->is_truthy()) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, nullptr);
        item = item->send(env, "succ");
    }
    return self;
}

Value *Range_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    RangeValue *self = self_value->as_range();
    if (self->exclude_end()) {
        return StringValue::sprintf(env, "%v...%v", self->begin(), self->end());
    } else {
        return StringValue::sprintf(env, "%v..%v", self->begin(), self->end());
    }
}

Value *Range_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    RangeValue *self = self_value->as_range();
    if (args[0]->type == Value::Type::Range) {
        RangeValue *arg = args[0]->as_range();
        Value *begin = arg->begin();
        Value *end = arg->end();
        bool begin_equal = self->begin()->send(env, "==", 1, &begin, nullptr)->is_truthy();
        bool end_equal = self->end()->send(env, "==", 1, &end, nullptr)->is_truthy();
        if (begin_equal && end_equal && self->exclude_end() == arg->exclude_end()) {
            return NAT_TRUE;
        }
    }
    return NAT_FALSE;
}

Value *Range_eqeqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    RangeValue *self = self_value->as_range();
    Value *arg = args[0];
    if (self->begin()->type == Value::Type::Integer && arg->type == Value::Type::Integer) {
        // optimized path for integer ranges
        int64_t begin = self->begin()->as_integer()->to_int64_t();
        int64_t end = self->end()->as_integer()->to_int64_t();
        int64_t val = arg->as_integer()->to_int64_t();
        if (begin <= val && ((self->exclude_end() && val < end) || (!self->exclude_end() && val <= end))) {
            return NAT_TRUE;
        }
    } else {
        // slower method that should work for any type of range
        Value *item = self->begin();
        const char *op = self->exclude_end() ? "<" : "<=";
        Value *end = self->end();
        while (item->send(env, op, 1, &end, nullptr)->is_truthy()) {
            if (item->send(env, "==", 1, &arg, nullptr)->is_truthy()) {
                return NAT_TRUE;
            }
            item = item->send(env, "succ");
        }
    }
    return NAT_FALSE;
}

}
