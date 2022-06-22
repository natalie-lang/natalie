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

Args::Args(ArrayObject &a)
    : m_size { a.size() }
    , m_data { a.data() } { }

Args::Args(const Args &other)
    : m_size { other.m_size }
    , m_data { other.m_data } { }

Args &Args::operator=(const Args &other) {
    m_size = other.m_size;
    m_data = other.m_data;
    return *this;
}

Args Args::shift(Args &args) {
    return Args { args.m_size - 1, args.m_data + 1 };
}

ArrayObject *Args::to_array() const {
    return new ArrayObject { m_size, m_data };
}

ArrayObject *Args::to_array_for_block(Env *env) const {
    if (m_size == 1)
        return to_ary(env, m_data[0], true)->dup(env)->as_array();
    return new ArrayObject { m_size, m_data };
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
