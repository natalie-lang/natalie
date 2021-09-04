#pragma once

namespace TM {

template <typename F>
class Defer {
public:
    Defer(F callback)
        : m_callback { callback } { }

    ~Defer() {
        m_callback();
    }

private:
    F m_callback;
};

}
