#include "natalie.h"
#include "builtin.h"

NatObject *MatchData_size(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_MATCHDATA);
    assert(self->matchdata_region->num_regs > 0);
    return nat_integer(env, self->matchdata_region->num_regs);
}

NatObject *MatchData_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_MATCHDATA);
    assert(self->matchdata_region->num_regs > 0);
    char *str = &self->matchdata_str[self->matchdata_region->beg[0]];
    NatObject *str_obj = nat_string(env, str);
    str_obj->str_len = self->matchdata_region->end[0] - self->matchdata_region->beg[0];
    str_obj->str[str_obj->str_len] = 0;
    return str_obj;
}

NatObject *MatchData_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_MATCHDATA);
    if (NAT_TYPE(args[0]) == NAT_VALUE_STRING || NAT_TYPE(args[0]) == NAT_VALUE_SYMBOL) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_INTEGER, "Integer");
    int64_t index = NAT_INT_VALUE(args[0]);
    assert(index >= 0); // TODO: accept negative indices
    if (index == 0) {
        return MatchData_to_s(env, self, 0, NULL, NULL, NULL);
    } else if (index >= self->matchdata_region->num_regs) {
        return NAT_NIL;
    } else {
        char *str = &self->matchdata_str[self->matchdata_region->beg[index]];
        NatObject *str_obj = nat_string(env, str);
        str_obj->str_len = self->matchdata_region->end[index] - self->matchdata_region->beg[index];
        str_obj->str[str_obj->str_len] = 0;
        return str_obj;
    }
}
