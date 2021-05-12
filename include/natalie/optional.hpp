#pragma once

#include "natalie/forward.hpp"
#include <assert.h>

namespace Natalie {

template <typename T>
class Optional {
public:
    Optional(T value)
        : m_present { true }
        , m_value { value } { }

    Optional()
        : m_present { false } { }

    T value() const {
        assert(m_present);
        return m_value;
    }

    T operator*() const {
        assert(m_present);
        return m_value;
    }

    operator bool() const { return m_present; }

private:
    bool m_present;
    T m_value;
};

}
