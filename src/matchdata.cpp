#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *MatchData_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    MatchDataValue *self = self_value->as_match_data();
    assert(self->matchdata_region->num_regs > 0);
    return new IntegerValue { env, self->matchdata_region->num_regs };
}

Value *MatchData_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    MatchDataValue *self = self_value->as_match_data();
    assert(self->matchdata_region->num_regs > 0);
    const char *str = &self->matchdata_str[self->matchdata_region->beg[0]];
    ssize_t length = self->matchdata_region->end[0] - self->matchdata_region->beg[0];
    return new StringValue { env, str, length };
}

Value *MatchData_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    MatchDataValue *self = self_value->as_match_data();
    if (NAT_TYPE(args[0]) == Value::Type::String || NAT_TYPE(args[0]) == Value::Type::Symbol) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
    int64_t index = args[0]->as_integer()->to_int64_t();
    assert(index >= 0); // TODO: accept negative indices
    if (index == 0) {
        return MatchData_to_s(env, self, 0, NULL, NULL);
    } else if (index >= self->matchdata_region->num_regs) {
        return NAT_NIL;
    } else {
        const char *str = &self->matchdata_str[self->matchdata_region->beg[index]];
        ssize_t length = self->matchdata_region->end[index] - self->matchdata_region->beg[index];
        return new StringValue { env, str, length };
    }
}

}
