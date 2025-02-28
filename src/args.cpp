#include "natalie.hpp"

namespace Natalie {

Value Args::operator[](size_t index) const {
    return (*tl_current_arg_stack)[m_args_start_index + index];
}

Value Args::at(size_t index) const {
    return tl_current_arg_stack->at(m_args_start_index + index);
}

Value Args::at(size_t index, Value default_value) const {
    auto result = maybe_at(index);
    if (result)
        return result.value();
    return default_value;
}

Optional<Value> Args::maybe_at(size_t index) const {
    auto offset = m_args_start_index + index;
    if (offset >= tl_current_arg_stack->size() || offset >= m_args_start_index + m_args_size)
        return Optional<Value>();
    return (*tl_current_arg_stack)[offset];
}

Args::Args(size_t size, const Value *data, bool has_keyword_hash)
    : m_args_size { size }
    , m_has_keyword_hash { has_keyword_hash } {
    for (size_t i = 0; i < size; i++)
        tl_current_arg_stack->push(data[i]);
}

Args::Args(const TM::Vector<Value> &vec, bool has_keyword_hash)
    : m_args_size { vec.size() }
    , m_has_keyword_hash { has_keyword_hash } {
    for (auto arg : vec)
        tl_current_arg_stack->push(arg);
}

Args::Args(ArrayObject *array, bool has_keyword_hash)
    : m_args_size { array->size() }
    , m_has_keyword_hash { has_keyword_hash } {
    for (Value arg : *array)
        tl_current_arg_stack->push(arg);
}

Args::Args(std::initializer_list<Value> args, bool has_keyword_hash)
    : m_args_original_start_index { tl_current_arg_stack->size() }
    , m_args_start_index { tl_current_arg_stack->size() }
    , m_args_size { args.size() }
    , m_has_keyword_hash { has_keyword_hash } {
    for (Value arg : args)
        tl_current_arg_stack->push(arg);
}

Args::Args(Args &other)
    : m_args_size { other.m_args_size }
    , m_has_keyword_hash { other.m_has_keyword_hash } {
    for (size_t i = 0; i < other.size(); i++)
        tl_current_arg_stack->push(other[i]);
}

Value Args::shift() {
    auto value = first();
    m_args_start_index++;
    m_args_size--;
    return value;
}

Value Args::pop() {
    auto value = last();
    m_args_size--;
    return value;
}

Value Args::first() const {
    assert(m_args_size > 0);
    return (*tl_current_arg_stack)[m_args_start_index];
}

Value Args::last() const {
    assert(m_args_size > 0);
    return (*tl_current_arg_stack)[m_args_start_index + m_args_size - 1];
}

ArrayObject *Args::to_array() const {
    return new ArrayObject { m_args_size, tl_current_arg_stack->data() + m_args_start_index };
}

ArrayObject *Args::to_array_for_block(Env *env, ssize_t min_count, ssize_t max_count, bool spread) const {
    if (m_args_size == 1 && spread) {
        auto ary = to_ary(env, at(0), true)->duplicate(env).as_array();
        ssize_t count = ary->size();
        if (max_count != -1 && count > max_count)
            ary->truncate(max_count);
        else if (count < min_count)
            ary->fill(env, Value::nil(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
        return ary;
    }
    auto len = max_count >= 0 ? std::min(m_args_size, (size_t)max_count) : m_args_size;
    auto ary = new ArrayObject { len, tl_current_arg_stack->data() + m_args_start_index };
    ssize_t count = ary->size();
    if (count < min_count)
        ary->fill(env, Value::nil(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
    return ary;
}

void Args::ensure_argc_is(Env *env, size_t expected, std::initializer_list<const String> keywords) const {
    if (m_args_size != expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}{})", m_args_size, expected, argc_error_suffix(keywords));
}

void Args::ensure_argc_between(Env *env, size_t expected_low, size_t expected_high, std::initializer_list<const String> keywords) const {
    if (m_args_size < expected_low || m_args_size > expected_high)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}..{}{})", m_args_size, expected_low, expected_high, argc_error_suffix(keywords));
}

void Args::ensure_argc_at_least(Env *env, size_t expected, std::initializer_list<const String> keywords) const {
    if (m_args_size < expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}+{})", m_args_size, expected, argc_error_suffix(keywords));
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

Value *Args::data() const {
    return &tl_current_arg_stack->data()[m_args_start_index];
}

HashObject *Args::keyword_hash() const {
    if (!m_has_keyword_hash || m_args_size == 0)
        return nullptr;

    auto hash = last();
    if (!hash || !hash.is_hash())
        return nullptr;

    return hash.as_hash();
}

HashObject *Args::pop_keyword_hash() {
    auto hash = keyword_hash();
    if (!hash)
        return nullptr;

    m_args_size--;
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
        return Value::nil();
    return hash->get(env, name).value_or(Value::nil());
}

}
