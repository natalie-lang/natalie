#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Range_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(2, 3);
    bool exclude_end = false;
    if (argc == 3) {
        exclude_end = truthy(args[2]);
    }
    return range(env, args[0], args[1], exclude_end);
}

NatObject *Range_begin(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    return self->range_begin;
}

NatObject *Range_end(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    return self->range_end;
}

NatObject *Range_exclude_end(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    return self->range_exclude_end ? NAT_TRUE : NAT_FALSE;
}

NatObject *Range_to_a(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    NatObject *ary = array_new(env);
    NatObject *item = self->range_begin;
    const char *op = self->range_exclude_end ? "<" : "<=";
    while (truthy(send(env, item, op, 1, &self->range_end, NULL))) {
        array_push(env, ary, item);
        item = send(env, item, "succ", 0, NULL, NULL);
    }
    return ary;
}

NatObject *Range_each(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    NatObject *item = self->range_begin;
    const char *op = self->range_exclude_end ? "<" : "<=";
    while (truthy(send(env, item, op, 1, &self->range_end, NULL))) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL);
        item = send(env, item, "succ", 0, NULL, NULL);
    }
    return self;
}

NatObject *Range_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    if (self->range_exclude_end) {
        return sprintf(env, "%v...%v", self->range_begin, self->range_end);
    } else {
        return sprintf(env, "%v..%v", self->range_begin, self->range_end);
    }
}

NatObject *Range_eqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_RANGE && truthy(send(env, self->range_begin, "==", 1, &arg->range_begin, NULL)) && truthy(send(env, self->range_end, "==", 1, &arg->range_end, NULL)) && self->range_exclude_end == arg->range_exclude_end) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Range_eqeqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    NatObject *arg = args[0];
    if (NAT_TYPE(self->range_begin) == NAT_VALUE_INTEGER && NAT_TYPE(arg) == NAT_VALUE_INTEGER) {
        // optimized path for integer ranges
        if (NAT_INT_VALUE(self->range_begin) <= NAT_INT_VALUE(arg) && ((self->range_exclude_end && NAT_INT_VALUE(arg) < NAT_INT_VALUE(self->range_end)) || (!self->range_exclude_end && NAT_INT_VALUE(arg) <= NAT_INT_VALUE(self->range_end)))) {
            return NAT_TRUE;
        }
    } else {
        // slower method that should work for any type of range
        NatObject *item = self->range_begin;
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
