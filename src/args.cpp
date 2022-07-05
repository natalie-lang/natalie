#include "natalie.hpp"

namespace Natalie {

Value Args::operator[](size_t index) const {
    // TODO: remove the assertion here once we're
    // in a good place with the transition to Args
    assert(index < m_size);
    return m_data[index];
}

Value Args::at(size_t index) const {
    assert(index < m_size);
    return m_data[index];
}

Value Args::at(size_t index, Value default_value) const {
    if (index >= m_size)
        return default_value;
    return m_data[index];
}

Args::Args(ArrayObject *array, Block *block, bool has_keyword_hash)
    : m_size { array->size() }
    , m_data { array->data() }
    , m_block { block }
    , m_has_keyword_hash { has_keyword_hash }
    , m_array_pointer_so_the_gc_does_not_collect_it { array } { }

Args::Args(const Args &other, Block *block)
    : m_size { other.m_size }
    , m_data { other.m_data }
    , m_block { block ?: other.m_block }
    , m_has_keyword_hash { other.m_has_keyword_hash }
    , m_array_pointer_so_the_gc_does_not_collect_it { other.m_array_pointer_so_the_gc_does_not_collect_it } { }

Args &Args::operator=(const Args &other) {
    m_size = other.m_size;
    m_data = other.m_data;
    m_block = other.m_block;
    m_has_keyword_hash = other.m_has_keyword_hash;
    m_array_pointer_so_the_gc_does_not_collect_it = other.m_array_pointer_so_the_gc_does_not_collect_it;
    return *this;
}

Args Args::shift(Args &args) {
    return Args { args.m_size - 1, args.m_data + 1, args.m_block };
}

ArrayObject *Args::to_array() const {
    return new ArrayObject { m_size, m_data };
}

ArrayObject *Args::to_array_for_block(Env *env, ssize_t min_count, ssize_t max_count) const {
    if (m_size == 1 && max_count > 1) {
        auto ary = to_ary(env, m_data[0], true)->dup(env)->as_array();
        ssize_t count = ary->size();
        if (count > max_count)
            ary->truncate(max_count);
        else if (count < min_count)
            ary->fill(env, NilObject::the(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
        return ary;
    }
    auto size = max_count >= 0 ? std::min(m_size, (size_t)max_count) : m_size;
    auto ary = new ArrayObject { size, m_data };
    ssize_t count = ary->size();
    if (count < min_count)
        ary->fill(env, NilObject::the(), Value::integer(ary->size()), Value::integer(min_count - ary->size()), nullptr);
    return ary;
}

void Args::ensure_argc_is(Env *env, size_t expected) const {
    if (m_size != expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", m_size, expected);
}

void Args::ensure_argc_between(Env *env, size_t expected_low, size_t expected_high) const {
    if (m_size < expected_low || m_size > expected_high)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}..{})", m_size, expected_low, expected_high);
}

void Args::ensure_argc_at_least(Env *env, size_t expected) const {
    if (m_size < expected)
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected {}+)", m_size, expected);
}

}
