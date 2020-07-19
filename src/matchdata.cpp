#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *MatchData_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    MatchDataValue *self = self_value->as_match_data();
    assert(self->size() > 0);
    return new IntegerValue { env, self->size() };
}

Value *MatchData_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    MatchDataValue *self = self_value->as_match_data();
    assert(self->size() > 0);
    return self->group(env, 0);
}

Value *MatchData_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    MatchDataValue *self = self_value->as_match_data();
    if (args[0]->type() == Value::Type::String || args[0]->type() == Value::Type::Symbol) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
    int64_t index = args[0]->as_integer()->to_int64_t();
    if (index < 0) {
        index = self->size() + index;
    }
    if (index < 0) {
        return NAT_NIL;
    } else if (index == 0) {
        return MatchData_to_s(env, self, 0, nullptr, nullptr);
    } else {
        return self->group(env, index);
    }
}

}
