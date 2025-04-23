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
    , m_keyword_hash_index { has_keyword_hash ? static_cast<ssize_t>(m_args_start_index) + static_cast<ssize_t>(m_args_size) - 1 : -1 } {
    for (size_t i = 0; i < size; i++)
        tl_current_arg_stack->push(data[i]);
}

Args::Args(const TM::Vector<Value> &vec, bool has_keyword_hash)
    : m_args_size { vec.size() }
    , m_keyword_hash_index { has_keyword_hash ? static_cast<ssize_t>(m_args_start_index) + static_cast<ssize_t>(m_args_size) - 1 : -1 } {
    for (auto arg : vec)
        tl_current_arg_stack->push(arg);
}

Args::Args(ArrayObject *array, bool has_keyword_hash)
    : m_args_size { array->size() }
    , m_keyword_hash_index { has_keyword_hash ? static_cast<ssize_t>(m_args_start_index) + static_cast<ssize_t>(m_args_size) - 1 : -1 } {
    for (Value arg : *array)
        tl_current_arg_stack->push(arg);
}

Args::Args(std::initializer_list<Value> args, bool has_keyword_hash)
    : m_args_original_start_index { tl_current_arg_stack->size() }
    , m_args_start_index { tl_current_arg_stack->size() }
    , m_args_size { args.size() }
    , m_keyword_hash_index { has_keyword_hash ? static_cast<ssize_t>(m_args_start_index) + static_cast<ssize_t>(m_args_size) - 1 : -1 } {
    for (Value arg : args)
        tl_current_arg_stack->push(arg);
}

Args::Args(Args &other)
    : m_args_size { other.m_args_size }
    , m_keyword_hash_index { other.m_keyword_hash_index } {
    for (size_t i = 0; i < other.size(); i++)
        tl_current_arg_stack->push(other[i]);
}

Value Args::shift(Env *env, bool include_keyword_hash) {
    if (has_keyword_hash()) {
        if (include_keyword_hash && m_args_size == 1)
            return pop_keyword_hash();
        assert(m_args_size > 1);
    } else {
        assert(m_args_size > 0);
    }
    auto value = (*tl_current_arg_stack)[m_args_start_index];
    m_args_start_index++;
    m_args_size--;
    return value;
}

Value Args::pop(Env *env, bool include_keyword_hash) {
    size_t index;
    if (has_keyword_hash()) {
        if (include_keyword_hash)
            return pop_keyword_hash();
        assert(m_args_size > 1);
        index = m_args_start_index + m_args_size - 2;
    } else {
        assert(m_args_size > 0);
        index = m_args_start_index + m_args_size - 1;
    }
    auto value = (*tl_current_arg_stack)[index];
    m_args_size--;
    return value;
}

ArrayObject *Args::to_array(bool include_keyword_hash) const {
    return new ArrayObject { size(include_keyword_hash), tl_current_arg_stack->data() + m_args_start_index };
}

ArrayObject *Args::to_array_for_block(Env *env, ssize_t min_count, ssize_t max_count, bool spread, bool include_keyword_hash) const {
    auto count = size(include_keyword_hash);
    if (count == 1 && spread) {
        auto ary = to_ary(env, at(0), true)->duplicate(env).as_array();
        ssize_t ary_count = ary->size();
        if (max_count != -1 && ary_count > max_count)
            ary->truncate(max_count);
        else if (ary_count < min_count)
            ary->fill(env, Value::nil(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
        return ary;
    }
    auto len = max_count >= 0 ? std::min(count, (size_t)max_count) : count;
    auto ary = new ArrayObject { len, tl_current_arg_stack->data() + m_args_start_index };
    ssize_t ary_count = ary->size();
    if (ary_count < min_count)
        ary->fill(env, Value::nil(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
    return ary;
}

void Args::ensure_argc_is(Env *env, size_t expected, bool method_has_keywords, std::initializer_list<const String> keywords) const {
    auto count = size(!method_has_keywords);
    if (count != expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}{})", count, expected, argc_error_suffix(keywords));
}

void Args::ensure_argc_between(Env *env, size_t expected_low, size_t expected_high, bool method_has_keywords, std::initializer_list<const String> keywords) const {
    auto count = size(!method_has_keywords);
    if (count < expected_low || count > expected_high) {
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}..{}{})", count, expected_low, expected_high, argc_error_suffix(keywords));
    }
}

void Args::ensure_argc_at_least(Env *env, size_t expected, bool method_has_keywords, std::initializer_list<const String> keywords) const {
    auto count = size(!method_has_keywords);
    if (count < expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}+{})", count, expected, argc_error_suffix(keywords));
}

void Args::ensure_no_missing_keywords(Env *env, std::initializer_list<const String> keywords) const {
    auto hash = keyword_hash();
    env->ensure_no_missing_keywords(hash, keywords);
}

void Args::ensure_no_extra_keywords(Env *env) const {
    auto hash = keyword_hash();
    if (hash)
        env->ensure_no_extra_keywords(hash);
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
    if (!has_keyword_hash() || m_args_size == 0)
        return nullptr;

    return (*tl_current_arg_stack)[m_keyword_hash_index].as_hash();
}

HashObject *Args::pop_keyword_hash() {
    auto hash = keyword_hash();
    if (!hash)
        return nullptr;

    m_args_size--;
    m_keyword_hash_index = -1;
    return hash;
}

void Args::pop_empty_keyword_hash() {
    if (!has_keyword_hash())
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

bool Args::keyword_arg_present(Env *env, SymbolObject *name) const {
    auto hash = keyword_hash();
    if (!hash)
        return false;
    return hash->get(env, name).present();
}

}
