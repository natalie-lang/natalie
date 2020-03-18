#include "natalie.h"
#include "builtin.h"

NatObject *Range_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(2, 3);
    bool exclude_end = false;
    if (argc == 3) {
        exclude_end = nat_truthy(args[2]);
    }
    return nat_range(env, args[0], args[1], exclude_end);
}

NatObject *Range_begin(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    return self->range_begin;
}

NatObject *Range_end(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    return self->range_end;
}

NatObject *Range_exclude_end(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    return self->range_exclude_end ? NAT_TRUE : NAT_FALSE;
}

NatObject *Range_to_a(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    NatObject *ary = nat_array(env);
    NatObject *item = self->range_begin;
    char *operator = self->range_exclude_end ? "<" : "<=";
    while (nat_truthy(nat_send(env, item, operator, 1, &self->range_end, NULL))) {
        nat_array_push(ary, item);
        item = nat_send(env, item, "succ", 0, NULL, NULL);
    }
    return ary;
}

NatObject *Range_each(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    NatObject *item = self->range_begin;
    char *operator = self->range_exclude_end ? "<" : "<=";
    while (nat_truthy(nat_send(env, item, operator, 1, &self->range_end, NULL))) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &item, NULL, NULL);
        item = nat_send(env, item, "succ", 0, NULL, NULL);
    }
    return self;
}

NatObject *Range_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_RANGE);
    if (self->range_exclude_end) {
        return nat_sprintf(env, "%v...%v", self->range_begin, self->range_end);
    } else {
        return nat_sprintf(env, "%v..%v", self->range_begin, self->range_end);
    }
}

