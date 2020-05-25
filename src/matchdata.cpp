#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *MatchData_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_MATCHDATA);
    assert(self->matchdata_region->num_regs > 0);
    return integer(env, self->matchdata_region->num_regs);
}

Value *MatchData_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_MATCHDATA);
    assert(self->matchdata_region->num_regs > 0);
    const char *str = &self->matchdata_str[self->matchdata_region->beg[0]];
    Value *str_obj = string(env, str);
    str_obj->str_len = self->matchdata_region->end[0] - self->matchdata_region->beg[0];
    str_obj->str[str_obj->str_len] = 0;
    return str_obj;
}

Value *MatchData_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_MATCHDATA);
    if (NAT_TYPE(args[0]) == NAT_VALUE_STRING || NAT_TYPE(args[0]) == NAT_VALUE_SYMBOL) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_INTEGER, "Integer");
    int64_t index = NAT_INT_VALUE(args[0]);
    assert(index >= 0); // TODO: accept negative indices
    if (index == 0) {
        return MatchData_to_s(env, self, 0, NULL, NULL);
    } else if (index >= self->matchdata_region->num_regs) {
        return NAT_NIL;
    } else {
        const char *str = &self->matchdata_str[self->matchdata_region->beg[index]];
        Value *str_obj = string(env, str);
        str_obj->str_len = self->matchdata_region->end[index] - self->matchdata_region->beg[index];
        str_obj->str[str_obj->str_len] = 0;
        return str_obj;
    }
}

}
