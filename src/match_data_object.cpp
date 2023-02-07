#include "natalie.hpp"

namespace Natalie {

Value MatchDataObject::array(int start) {
    auto size = (size_t)(m_region->num_regs - start);
    auto array = new ArrayObject { size };
    for (int i = start; i < m_region->num_regs; i++) {
        array->push(group(i));
    }
    return array;
}

ssize_t MatchDataObject::index(size_t index) {
    if (index >= size()) return -1;
    return m_region->beg[index];
}

ssize_t MatchDataObject::ending(size_t index) {
    if (index >= size()) return -1;
    return m_region->end[index];
}

/**
 * Return the capture indicated by the given index. This function supports
 * negative indices as well, provided they are within one overall length of the
 * number of captures.
 */
Value MatchDataObject::group(int index) {
    if (index + m_region->num_regs <= 0 || index >= m_region->num_regs)
        return NilObject::the();

    if (index < 0)
        index += m_region->num_regs;

    if (m_region->beg[index] == -1)
        return NilObject::the();

    const char *str = &m_string->c_str()[m_region->beg[index]];
    size_t length = m_region->end[index] - m_region->beg[index];
    return new StringObject { str, length };
}

Value MatchDataObject::offset(Env *env, Value n) {
    nat_int_t index = IntegerObject::convert_to_nat_int_t(env, n);
    if (index >= (nat_int_t)size())
        return NilObject::the();

    auto begin = m_region->beg[index];
    auto end = m_region->end[index];
    if (begin == -1)
        return new ArrayObject { NilObject::the(), NilObject::the() };

    auto chars = m_string->chars(env)->as_array();
    return new ArrayObject {
        Value::integer(StringObject::byte_index_to_char_index(chars, begin)),
        Value::integer(StringObject::byte_index_to_char_index(chars, end))
    };
}

Value MatchDataObject::captures(Env *env) {
    return this->array(1);
}

Value MatchDataObject::inspect(Env *env) {
    StringObject *out = new StringObject { "#<MatchData" };
    for (int i = 0; i < m_region->num_regs; i++) {
        out->append_char(' ');
        if (i > 0) {
            out->append(i);
            out->append_char(':');
        }
        out->append(this->group(i)->inspect_str(env));
    }
    out->append_char('>');
    return out;
}

Value MatchDataObject::match(Env *env, Value index) {
    if (!index->is_integer()) {
        env->raise("TypeError", "no implicit conversion of {} into Integer", index->klass()->inspect_str());
    }
    auto match = this->group(IntegerObject::convert_to_int(env, index));
    if (match->is_nil()) {
        return NilObject::the();
    }
    return match;
}

Value MatchDataObject::match_length(Env *env, Value index) {
    auto match = this->match(env, index);
    if (match->is_nil()) {
        return match;
    }
    return match->as_string()->size(env);
}

Value MatchDataObject::post_match(Env *env) {
    if (m_region->beg[0] == -1)
        return NilObject::the();
    return m_string->ref_fast_range(env, m_region->end[0], m_string->length());
}

Value MatchDataObject::to_a(Env *env) {
    return this->array(0);
}

Value MatchDataObject::to_s(Env *env) {
    assert(size() > 0);
    return group(0);
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
    return group(index);
}

}
