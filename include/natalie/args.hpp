#pragma once

#include "tm/vector.hpp"
#include <stddef.h>

namespace Natalie {

class ArrayObject;
class Env;
class Value;

class Args {
public:
    Args() { }

    Args(size_t size, const Value *data, bool owned = false)
        : m_size { size }
        , m_data { data } { }

    Args(TM::Vector<Value> vec)
        : m_size { vec.size() }
        , m_data { vec.data() } { }

    Args(ArrayObject &a);

    Args(std::initializer_list<Value> args)
        : m_size { args.size() }
        , m_data { std::data(args) } { }

    Args(const Args &other);

    Args(Args &&other)
        : m_size { other.m_size }
        , m_data { other.m_data } {
        other.m_size = 0;
        other.m_data = nullptr;
    }

    Args &operator=(const Args &other);

    static Args shift(Args &args);

    Value operator[](size_t index) const;

    Value at(size_t index) const;
    Value at(size_t index, Value default_value) const;

    ArrayObject *to_array() const;
    ArrayObject *to_array_for_block(Env *env) const;

    void ensure_argc_is(Env *env, size_t expected) const;
    void ensure_argc_between(Env *env, size_t expected_low, size_t expected_high) const;
    void ensure_argc_at_least(Env *env, size_t expected) const;

    size_t size() const { return m_size; }

    const Value *data() const { return m_data; }

private:
    size_t m_size { 0 };
    const Value *m_data { nullptr };
};
};
