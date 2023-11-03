#include "natalie.hpp"

namespace Natalie {

Value Args::operator[](size_t index) const {
    // TODO: remove the assertion here once we're
    // in a good place with the transition to Args
    assert(index < m_data.size());
    return m_data[index];
}

Value Args::at(size_t index) const {
    assert(index < m_data.size());
    return m_data.at(index);
}

Value Args::at(size_t index, Value default_value) const {
    if (index >= m_data.size())
        return default_value;
    return m_data[index];
}

Args::Args(size_t size, const Value *data, bool has_keyword_hash)
    : m_data { size }
    , m_has_keyword_hash { has_keyword_hash } {
    for (size_t i = 0; i < size; i++)
        m_data.push(data[i]);
}

Args::Args(ArrayObject *array, bool has_keyword_hash)
    : m_data { array->size() }
    , m_has_keyword_hash { has_keyword_hash } {
    for (Value e : *array)
        m_data.push(e);
}

Args::Args(const Args &other)
    : m_data { other.m_data }
    , m_has_keyword_hash { other.m_has_keyword_hash } { }

Args &Args::operator=(const Args &other) {
    m_data = Vector<Value> { other.m_data };
    m_has_keyword_hash = other.m_has_keyword_hash;
    return *this;
}

Args Args::shift(Args &args) {
    assert(args.size() > 0);
    if (args.size() == 1)
        return Args();
    auto ary = new ArrayObject(args.size() - 1, args.data() + 1);
    return Args(ary, args.has_keyword_hash());
}

Value Args::shift() {
    assert(m_data.size() > 0);
    return m_data.pop_front();
}

ArrayObject *Args::to_array() const {
    return new ArrayObject { m_data.size(), m_data.data() };
}

ArrayObject *Args::to_array_for_block(Env *env, ssize_t min_count, ssize_t max_count, bool spread) const {
    if (m_data.size() == 1 && spread) {
        auto ary = to_ary(env, m_data[0], true)->dup(env)->as_array();
        ssize_t count = ary->size();
        if (max_count != -1 && count > max_count)
            ary->truncate(max_count);
        else if (count < min_count)
            ary->fill(env, NilObject::the(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
        return ary;
    }
    auto len = max_count >= 0 ? std::min(m_data.size(), (size_t)max_count) : m_data.size();
    auto ary = new ArrayObject { len, m_data.data() };
    ssize_t count = ary->size();
    if (count < min_count)
        ary->fill(env, NilObject::the(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
    return ary;
}

void Args::ensure_argc_is(Env *env, size_t expected, std::initializer_list<const String> keywords) const {
    if (m_data.size() != expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}{})", m_data.size(), expected, argc_error_suffix(keywords));
}

void Args::ensure_argc_between(Env *env, size_t expected_low, size_t expected_high, std::initializer_list<const String> keywords) const {
    if (m_data.size() < expected_low || m_data.size() > expected_high)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}..{}{})", m_data.size(), expected_low, expected_high, argc_error_suffix(keywords));
}

void Args::ensure_argc_at_least(Env *env, size_t expected, std::initializer_list<const String> keywords) const {
    if (m_data.size() < expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}+{})", m_data.size(), expected, argc_error_suffix(keywords));
}

String Args::argc_error_suffix(std::initializer_list<const String> keywords) const {
    if (std::empty(keywords))
        return "";
    auto kw_data = std::data(keywords);
    if (keywords.size() == 1)
        return String::format("; required keyword: {}", kw_data[0]);
    String out { "; required keywords: " };
    out.append(kw_data[0]);
    for (size_t i = 1; i < keywords.size(); i++) {
        out.append(", ");
        out.append(kw_data[i]);
    }
    return out;
}

HashObject *Args::keyword_hash() const {
    if (!m_has_keyword_hash || m_data.is_empty())
        return nullptr;
    auto hash = m_data.last().object_or_null();
    if (!hash || !hash->is_hash())
        return nullptr;
    return hash->as_hash();
}

HashObject *Args::pop_keyword_hash() {
    auto hash = keyword_hash();
    if (!hash)
        return nullptr;
    m_data.set_size(m_data.size() - 1);
    m_has_keyword_hash = false;
    return hash;
}

void Args::pop_empty_keyword_hash() {
    if (!m_has_keyword_hash)
        return;
    auto hash = keyword_hash();
    if (hash && hash->is_empty())
        pop_keyword_hash();
}

Value Args::keyword_arg(Env *env, SymbolObject *name) const {
    auto hash = keyword_hash();
    if (!hash)
        return NilObject::the();
    return hash->get(env, name);
}
}
