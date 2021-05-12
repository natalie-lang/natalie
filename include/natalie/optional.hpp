#pragma once

#include "natalie/forward.hpp"
#include <assert.h>
#include <cstdio>

namespace Natalie {

template <typename T>
class Optional {
public:
    Optional(T value)
        : m_present { true }
        , m_value { value } { }

    Optional()
        : m_present { false } { }

    Optional(const Optional &other)
        : m_present { other.m_present } {
        if (m_present)
            m_value = other.m_value;
    }

    Optional<T> &operator=(const Optional<T> &other) {
        m_present = other.m_present;
        if (m_present)
            m_value = other.m_value;
        return *this;
    }

    ~Optional() {
        clear();
    }

    T &value() {
        assert(m_present);
        return m_value;
    }

    T operator*() {
        assert(m_present);
        return m_value;
    }

    Optional<T> &operator=(T &&value) {
        m_present = true;
        m_value = value;
        return *this;
    }

    void clear() { m_present = false; }

    operator bool() const { return m_present; }

private:
    bool m_present;

    union {
        T m_value;
    };
};
}
