#include "natalie.hpp"

namespace Natalie {

size_t MatchDataObject::index(size_t index) {
    if (index >= size()) return -1;
    return m_region->beg[index];
}

Value MatchDataObject::group(Env *env, size_t index) {
    if (index >= size()) return NilObject::the();
    const char *str = &m_string->c_str()[m_region->beg[index]];
    size_t length = m_region->end[index] - m_region->beg[index];
    return new StringObject { str, length };
}

Value MatchDataObject::to_s(Env *env) {
    assert(size() > 0);
    return group(env, 0);
}

Value MatchDataObject::ref(Env *env, Value index_value) {
    if (index_value->type() == Object::Type::String || index_value->type() == Object::Type::Symbol) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    index_value->assert_type(env, Object::Type::Integer, "Integer");
    nat_int_t index = index_value->as_integer()->to_nat_int_t();
    if (index < 0) {
        index = size() + index;
    }
    if (index < 0) {
        return NilObject::the();
    } else {
        return group(env, index);
    }
}

}
