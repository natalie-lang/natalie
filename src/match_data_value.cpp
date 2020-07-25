#include "natalie.hpp"

namespace Natalie {

ssize_t MatchDataValue::index(ssize_t index) {
    if (index >= size()) return -1;
    return m_region->beg[index];
}

Value *MatchDataValue::group(Env *env, ssize_t index) {
    if (index >= size()) return env->nil_obj();
    const char *str = &m_str[m_region->beg[index]];
    ssize_t length = m_region->end[index] - m_region->beg[index];
    return new StringValue { env, str, length };
}

Value *MatchDataValue::to_s(Env *env) {
    assert(size() > 0);
    return group(env, 0);
}

Value *MatchDataValue::ref(Env *env, Value *index_value) {
    if (index_value->type() == Value::Type::String || index_value->type() == Value::Type::Symbol) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    NAT_ASSERT_TYPE(index_value, Value::Type::Integer, "Integer");
    int64_t index = index_value->as_integer()->to_int64_t();
    if (index < 0) {
        index = size() + index;
    }
    if (index < 0) {
        return env->nil_obj();
    } else {
        return group(env, index);
    }
}

}
