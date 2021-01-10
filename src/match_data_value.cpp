#include "natalie.hpp"

namespace Natalie {

size_t MatchDataValue::index(size_t index) {
    if (index >= size()) return -1;
    return m_region->beg[index];
}

ValuePtr MatchDataValue::group(Env *env, size_t index) {
    if (index >= size()) return env->nil_obj();
    const char *str = &m_str[m_region->beg[index]];
    size_t length = m_region->end[index] - m_region->beg[index];
    return new StringValue { env, str, length };
}

ValuePtr MatchDataValue::to_s(Env *env) {
    assert(size() > 0);
    return group(env, 0);
}

ValuePtr MatchDataValue::ref(Env *env, ValuePtr index_value) {
    if (index_value->type() == Value::Type::String || index_value->type() == Value::Type::Symbol) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    index_value->assert_type(env, Value::Type::Integer, "Integer");
    nat_int_t index = index_value->as_integer()->to_nat_int_t();
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
