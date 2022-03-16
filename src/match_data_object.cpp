#include "natalie.hpp"

namespace Natalie {

Value MatchDataObject::array(int start) {
    assert(start < m_region->num_regs);
    auto size = (size_t)(m_region->num_regs - start);
    auto array = new ArrayObject { size };
    for (int i = start; i < m_region->num_regs; i++) {
        if (m_region->beg[i] == -1) {
            array->push(NilObject::the());
        } else {
            const char *str = &m_string->c_str()[m_region->beg[i]];
            size_t length = m_region->end[i] - m_region->beg[i];
            array->push(new StringObject { str, length });
        }
    }
    return array;
}

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

Value MatchDataObject::captures(Env *env) {
    return this->array(1);
}

Value MatchDataObject::to_a(Env *env) {
    return this->array(0);
}

Value MatchDataObject::to_s(Env *env) {
    assert(size() > 0);
    return group(env, 0);
}

Value MatchDataObject::ref(Env *env, Value index_value) {
    if (index_value->type() == Object::Type::String || index_value->type() == Object::Type::Symbol) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    nat_int_t index;
    if (index_value.is_fast_integer()) {
        index = index_value.get_fast_integer();
    } else {
        index = IntegerObject::convert_to_nat_int_t(env, index_value);
    }
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
