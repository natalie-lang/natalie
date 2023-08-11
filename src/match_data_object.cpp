#include "natalie.hpp"

namespace Natalie {

namespace {
    struct named_captures_data {
        const MatchDataObject *match_data_object;
        Env *env;
        HashObject *named_captures;
    };
};

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
Value MatchDataObject::group(int index) const {
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
        if (index->is_symbol()) {
            index = index->to_s(env);
        } else if (!index->is_string() && index->respond_to(env, "to_str"_s)) {
            index = index->send(env, "to_str"_s);
        }
        index->assert_type(env, Object::Type::String, "String");

        auto name = reinterpret_cast<const UChar *>(index->as_string()->c_str());
        const auto backref_number = onig_name_to_backref_number(m_regexp->m_regex, name, name + index->as_string()->bytesize(), m_region);

        return group(backref_number);
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

Value MatchDataObject::named_captures(Env *env) const {
    if (!m_regexp)
        return new HashObject {};

    auto named_captures = new HashObject {};
    named_captures_data data { this, env, named_captures };
    onig_foreach_name(
        m_regexp->m_regex,
        [](const UChar *name, const UChar *name_end, int groups_size, int *groups, regex_t *, void *data) -> int {
            auto match_data_object = (static_cast<named_captures_data *>(data))->match_data_object;
            auto env = (static_cast<named_captures_data *>(data))->env;
            auto named_captures = (static_cast<named_captures_data *>(data))->named_captures;
            const size_t length = name_end - name;
            // NATFIXME: Fully support character encodings in capture groups (see RegexpObject::initialize)
            auto key = new StringObject { reinterpret_cast<const char *>(name), length, EncodingObject::get(Encoding::UTF_8) };
            Value value = NilObject::the();
            for (int i = groups_size - 1; i >= 0; i--) {
                auto v = match_data_object->group(groups[i]);
                if (!v->is_nil()) {
                    value = v;
                    break;
                }
            }
            named_captures->put(env, key, value);
            return 0;
        },
        &data);
    return named_captures;
}

Value MatchDataObject::names() const {
    if (!m_regexp)
        return new ArrayObject {};
    return m_regexp->names();
}

Value MatchDataObject::post_match(Env *env) {
    if (m_region->beg[0] == -1)
        return NilObject::the();
    return m_string->ref_fast_range(env, m_region->end[0], m_string->length());
}

Value MatchDataObject::pre_match(Env *env) {
    if (m_region->beg[0] == -1)
        return NilObject::the();
    return m_string->ref_fast_range(env, 0, m_region->beg[0]);
}

Value MatchDataObject::regexp() const {
    if (!m_regexp) return NilObject::the();

    return m_regexp;
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
