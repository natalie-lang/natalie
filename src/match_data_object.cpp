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

size_t MatchDataObject::bytesize() const {
    if (m_region->num_regs == 0)
        return 0;

    return m_region->end[0] - m_region->beg[0];
}

Value MatchDataObject::byteoffset(Env *env, Value n) {
    nat_int_t index;
    if (n->is_string() || n->is_symbol()) {
        const auto &str = n->type() == Object::Type::String ? n->as_string()->string() : n->as_symbol()->string();
        index = onig_name_to_backref_number(m_regexp->m_regex, reinterpret_cast<const UChar *>(str.c_str()), reinterpret_cast<const UChar *>(str.c_str() + str.size()), m_region);
        if (index < 0)
            env->raise("IndexError", "undefined group name reference: {}", str);
    } else {
        index = n->to_int(env)->to_nat_int_t();
        if (index < 0 || index >= static_cast<nat_int_t>(size()))
            env->raise("IndexError", "index {} out of matches", index);
    }

    auto begin = m_region->beg[index];
    auto end = m_region->end[index];
    if (begin == -1)
        return new ArrayObject { NilObject::the(), NilObject::the() };

    return new ArrayObject { Value::integer(begin), Value::integer(end) };
}

ssize_t MatchDataObject::beg_byte_index(size_t index) const {
    if (index >= size()) return -1;
    return m_region->beg[index];
}

ssize_t MatchDataObject::beg_char_index(Env *env, size_t index) const {
    if (index >= size()) return -1;
    auto begin = m_region->beg[index];

    return m_string->byte_index_to_char_index(begin);
}

ssize_t MatchDataObject::end_byte_index(size_t index) const {
    if (index >= size()) return -1;
    return m_region->end[index];
}

ssize_t MatchDataObject::end_char_index(Env *env, size_t index) const {
    if (index >= size()) return -1;
    auto end = m_region->end[index];

    return m_string->byte_index_to_char_index(end);
}

bool MatchDataObject::is_empty() const {
    return m_region->beg[0] == m_region->end[0];
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

    assert(index >= 0);

    if (m_region->beg[index] == -1)
        return NilObject::the();

    const char *str = &m_string->c_str()[m_region->beg[index]];
    size_t length = m_region->end[index] - m_region->beg[index];
    return new StringObject { str, length, m_string->encoding() };
}

Value MatchDataObject::offset(Env *env, Value n) {
    nat_int_t index = IntegerObject::convert_to_nat_int_t(env, n);
    if (index >= (nat_int_t)size())
        return NilObject::the();

    ssize_t begin = m_region->beg[index];
    ssize_t end = m_region->end[index];
    if (begin == -1)
        return new ArrayObject { NilObject::the(), NilObject::the() };

    size_t current_byte_index = 0;
    size_t current_char_index = 0;
    TM::StringView view;
    while ((size_t)begin > current_byte_index) {
        view = m_string->next_char(&current_byte_index);
        current_char_index++;
        if (view.is_empty()) break;
    }
    size_t begin_char_index = current_char_index;

    while ((size_t)end > current_byte_index) {
        view = m_string->next_char(&current_byte_index);
        current_char_index++;
        if (view.is_empty()) break;
    }
    size_t end_char_index = current_char_index;

    return new ArrayObject { Value::integer(begin_char_index), Value::integer(end_char_index) };
}

Value MatchDataObject::begin(Env *env, Value start) const {
    nat_int_t index;
    if (start->is_string() || start->is_symbol()) {
        const auto &str = start->type() == Object::Type::String ? start->as_string()->string() : start->as_symbol()->string();
        index = onig_name_to_backref_number(m_regexp->m_regex, reinterpret_cast<const UChar *>(str.c_str()), reinterpret_cast<const UChar *>(str.c_str() + str.size()), m_region);
    } else {
        index = start->to_int(env)->to_nat_int_t();
    }
    if (index < 0)
        env->raise("IndexError", "bad index");
    if (group(index)->is_nil())
        return NilObject::the();
    return IntegerObject::from_ssize_t(env, beg_char_index(env, (size_t)index));
}

Value MatchDataObject::captures(Env *env) {
    return this->array(1);
}

Value MatchDataObject::end(Env *env, Value end) const {
    nat_int_t index;
    if (end->is_string() || end->is_symbol()) {
        const auto &str = end->type() == Object::Type::String ? end->as_string()->string() : end->as_symbol()->string();
        index = onig_name_to_backref_number(m_regexp->m_regex, reinterpret_cast<const UChar *>(str.c_str()), reinterpret_cast<const UChar *>(str.c_str() + str.size()), m_region);
    } else {
        index = end->to_int(env)->to_nat_int_t();
    }
    if (index < 0)
        env->raise("IndexError", "bad index");
    if (group(index)->is_nil())
        return NilObject::the();
    return IntegerObject::from_ssize_t(env, end_char_index(env, (size_t)index));
}

Value MatchDataObject::deconstruct_keys(Env *env, Value keys) {
    if (!keys || keys->is_nil()) {
        auto result = new HashObject {};
        for (auto name : *names()->as_array()) {
            auto value = ref(env, name);
            result->put(env, name->as_string()->to_sym(env), value);
        }
        return result;
    }

    if (!keys->is_array())
        env->raise("TypeError", "wrong argument type {} (expected Array)", keys->klass()->inspect_str());

    auto result = new HashObject {};
    if (keys->as_array()->size() > static_cast<size_t>(onig_number_of_names(m_regexp->m_regex)))
        return result;

    for (auto name : *keys->as_array()) {
        if (!name->is_symbol())
            env->raise("TypeError", "wrong argument type {} (expected Symbol)", name->klass()->inspect_str());
        const auto &str = name->as_symbol()->string();
        auto index = onig_name_to_backref_number(m_regexp->m_regex, reinterpret_cast<const UChar *>(str.c_str()), reinterpret_cast<const UChar *>(str.c_str() + str.size()), m_region);
        if (index < 0)
            break;
        result->put(env, name, group(index));
    }
    return result;
}

bool MatchDataObject::eq(Env *env, Value other) const {
    if (!other->is_match_data()) return false;
    const auto other_md = other->as_match_data();
    return m_string->eq(env, other_md->m_string) && m_regexp->eq(env, other_md->m_regexp);
}

Value MatchDataObject::inspect(Env *env) {
    StringObject *out = new StringObject { "#<MatchData" };
    const auto names_size = static_cast<size_t>(onig_number_of_names(m_regexp->m_regex));
    // NATFIXME: TM::StringView would work too, but we need a constructor from char *
    auto names = TM::Vector<TM::Optional<TM::String>> { names_size };
    onig_foreach_name(
        m_regexp->m_regex,
        [](const UChar *name, const UChar *name_end, int group_size, int *group, regex_t *, void *data) -> int {
            auto names = static_cast<TM::Vector<TM::Optional<TM::String>> *>(data);
            for (int i = 0; i < group_size; i++) {
                const size_t len = name_end - name;
                names->insert(group[i] - 1, TM::String(reinterpret_cast<const char *>(name), len));
            }
            return 0;
        },
        &names);

    for (int i = 0; i < m_region->num_regs; i++) {
        out->append_char(' ');
        if (i > 0) {
            if (static_cast<size_t>(i) <= names_size && names[i - 1]) {
                out->append(names[i - 1].value());
            } else {
                out->append(i);
            }
            out->append_char(':');
        }
        out->append(this->group(i)->inspect_str(env));
    }
    out->append_char('>');
    return out;
}

Value MatchDataObject::match(Env *env, Value index) {
    if (!index->is_integer()) {
        if (index->is_symbol())
            index = index->to_s(env);

        auto name = reinterpret_cast<const UChar *>(index->to_str(env)->c_str());
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
        [](const UChar *name, const UChar *name_end, int groups_size, int *groups, regex_t *regex, void *data) -> int {
            auto match_data_object = (static_cast<named_captures_data *>(data))->match_data_object;
            auto env = (static_cast<named_captures_data *>(data))->env;
            auto named_captures = (static_cast<named_captures_data *>(data))->named_captures;
            const size_t length = name_end - name;
            auto key = new StringObject { reinterpret_cast<const char *>(name), length, RegexpObject::onig_encoding_to_ruby_encoding(regex->enc) };
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

    auto length = m_string->bytesize() - m_region->end[0];
    if (length == 0)
        return new StringObject { "", m_string->encoding() };

    return new StringObject { m_string->string().substring(m_region->end[0], length), m_string->encoding() };
}

Value MatchDataObject::pre_match(Env *env) {
    if (m_region->beg[0] == -1)
        return NilObject::the();

    auto length = m_region->beg[0];
    if (length == 0)
        return new StringObject { "", m_string->encoding() };

    return new StringObject { m_string->string().substring(0, length), m_string->encoding() };
}

Value MatchDataObject::regexp() const {
    if (!m_regexp) return NilObject::the();

    return m_regexp;
}

Value MatchDataObject::to_a(Env *env) {
    return this->array(0);
}

Value MatchDataObject::to_s(Env *env) const {
    assert(size() > 0);
    return group(0);
}

ArrayObject *MatchDataObject::values_at(Env *env, Args args) {
    auto result = new ArrayObject {};
    for (size_t i = 0; i < args.size(); i++) {
        auto key = args[i];
        if (key->is_range()) {
            auto range = key->as_range();
            if (range->begin()->is_integer() && range->begin()->as_integer()->to_nat_int_t() < -static_cast<nat_int_t>(size()))
                env->raise("RangeError", "{} out of range", range->inspect_str(env));
            auto append = ref(env, range);
            result->concat(env, { append });
            auto size = range->send(env, "size"_s);
            if (append->is_array() && size->is_integer() && size->as_integer()->to_nat_int_t() > static_cast<nat_int_t>(append->as_array()->size())) {
                for (nat_int_t i = append->as_array()->size(); i < size->as_integer()->to_nat_int_t(); i++)
                    result->push(NilObject::the());
            }
        } else {
            result->push(ref(env, key));
        }
    }
    return result;
}

Value MatchDataObject::ref(Env *env, Value index_value, Value size_value) {
    if (index_value->type() == Object::Type::String || index_value->type() == Object::Type::Symbol) {
        const auto &str = index_value->type() == Object::Type::String ? index_value->as_string()->string() : index_value->as_symbol()->string();
        const nat_int_t index = onig_name_to_backref_number(m_regexp->m_regex, reinterpret_cast<const UChar *>(str.c_str()), reinterpret_cast<const UChar *>(str.c_str() + str.size()), m_region);

        if (index < 0)
            env->raise("IndexError", "undefined group name reference: {}", str);

        return group(index);
    }
    if (index_value->type() == Object::Type::Range) {
        auto range = index_value->as_range();
        const nat_int_t first = range->begin()->is_nil() ? 0 : IntegerObject::convert_to_nat_int_t(env, range->begin());
        const nat_int_t last = range->end()->is_nil() ? size() - 1 : IntegerObject::convert_to_nat_int_t(env, range->end());
        if (last < first)
            return new ArrayObject {};
        auto first_result = group(first);
        if (first_result->is_nil())
            return NilObject::the();
        auto result = new ArrayObject { first_result };
        for (auto i = first + 1; i <= last; i++) {
            auto next_result = group(i);
            if (next_result->is_nil())
                break;
            result->push(next_result);
        }
        return result;
    }
    nat_int_t index;
    if (index_value.is_fast_integer()) {
        index = index_value.get_fast_integer();
    } else {
        index = IntegerObject::convert_to_nat_int_t(env, index_value);
    }
    if (size_value && !size_value->is_nil()) {
        nat_int_t size;
        if (size_value.is_fast_integer()) {
            size = size_value.get_fast_integer();
        } else {
            size = IntegerObject::convert_to_nat_int_t(env, size_value);
        }

        if (size < 0)
            return NilObject::the();
        if (size == 0)
            return new ArrayObject {};

        auto first_result = group(index);
        if (first_result->is_nil())
            return NilObject::the();

        auto result = new ArrayObject { first_result };
        for (auto i = index + 1; i < index + size; i++) {
            auto next_result = group(i);
            if (next_result->is_nil()) break;
            result->push(next_result);
        }
        return result;
    }
    return group(index);
}

String MatchDataObject::dbg_inspect() const {
    auto str = group(0);
    assert(!str->is_nil());
    return String::format("#<MatchData \"{}\">", str->as_string()->c_str());
}

}
