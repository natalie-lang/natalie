#pragma once

#include <assert.h>
#include <stdio.h>

namespace TM {

template <typename T>
class Optional {
public:
    Optional(T value)
        : m_present { true }
        , m_value { value } { }

    Optional()
        : m_present { false }
        , m_value { nullptr } { }

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

    T const &value() const {
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

    operator bool() const {
        if (!m_present) assert(!m_value); // TODO: remove this once we fix our GC bugs
        return m_present;
    }

    bool present() const {
        return m_present;
    }

private:
    bool m_present;

    union {
        T m_value;
    };
};
}
