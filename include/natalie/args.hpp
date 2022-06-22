#pragma once

#include "tm/vector.hpp"
#include <stddef.h>

namespace Natalie {

class ArrayObject;
class Env;
class Value;

struct Args {
    Args() { }

    Args(size_t c, const Value *a, bool owned = false)
        : argc { c }
        , args { a } { }

    Args(TM::Vector<Value> a)
        : argc { a.size() }
        , args { a.data() } { }

    Args(ArrayObject &a);

    Args(std::initializer_list<Value> a)
        : argc { a.size() }
        , args { data(a) } { }

    Args(const Args &other);

    Args(Args &&other)
        : argc { other.argc }
        , args { other.args } {
        other.argc = 0;
        other.args = nullptr;
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

    // TODO: make these private and provide accessors
    size_t argc { 0 };
    const Value *args { nullptr };
};
};
