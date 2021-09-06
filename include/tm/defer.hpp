#pragma once

#include <utility>

namespace TM {

template <typename F>
class Defer {
public:
    Defer(F &&callback)
        : m_callback { std::forward<F>(callback) } { }

    ~Defer() {
        m_callback();
    }

private:
    F m_callback;
};

}
