#pragma once

#include "tm/macros.hpp"
#include "tm/string.hpp"
#include "tm/vector.hpp"
#include <iterator>
#include <stddef.h>
#include <stdio.h>

namespace Natalie {

using namespace TM;

class ArrayObject;
class Env;
class HashObject;
class SymbolObject;
class Value;

class Args {
public:
    Args() { }

    Args(size_t size, const Value *data, bool has_keyword_hash = false);

    Args(const TM::Vector<Value> &vec, bool has_keyword_hash = false)
        : m_data { vec }
        , m_has_keyword_hash { has_keyword_hash } { }

    Args(TM::Vector<Value> &&vec, bool has_keyword_hash = false)
        : m_data { std::move(vec) }
        , m_has_keyword_hash { has_keyword_hash } { }

    Args(ArrayObject *array, bool has_keyword_hash = false);

    Args(std::initializer_list<Value> args, bool has_keyword_hash = false)
        : m_data { args }
        , m_has_keyword_hash { has_keyword_hash } { }

    Args(const Args &other);

    Args(Args &&other)
        : m_data { std::move(other.m_data) }
        , m_has_keyword_hash { other.m_has_keyword_hash } {
        other.m_has_keyword_hash = false;
    }

    Args &operator=(const Args &other);

    static Args shift(Args &args);
    Value shift();

    Value operator[](size_t index) const;

    Value at(size_t index) const;
    Value at(size_t index, Value default_value) const;

    ArrayObject *to_array() const;
    ArrayObject *to_array_for_block(Env *env, ssize_t min_count, ssize_t max_count, bool spread) const;

    void ensure_argc_is(Env *env, size_t expected, std::initializer_list<const String> keywords = {}) const;
    void ensure_argc_between(Env *env, size_t expected_low, size_t expected_high, std::initializer_list<const String> keywords = {}) const;
    void ensure_argc_at_least(Env *env, size_t expected, std::initializer_list<const String> keywords = {}) const;

    size_t size() const { return m_data.size(); }
    const Value *data() const { return m_data.data(); }

    bool has_keyword_hash() const { return m_has_keyword_hash; }
    HashObject *keyword_hash() const;
    HashObject *pop_keyword_hash();
    void pop_empty_keyword_hash();
    Value keyword_arg(Env *, SymbolObject *) const;

private:
    // Args cannot be heap-allocated, because the GC is not aware of it.
    void *operator new(size_t size) = delete;

    String argc_error_suffix(std::initializer_list<const String> keywords) const;

    Vector<Value> m_data {};
    bool m_has_keyword_hash { false };
};
};
