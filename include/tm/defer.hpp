#pragma once

#include <utility>

namespace TM {

template <typename F>
class Defer {
public:
    /**
     * Constructs a Defer object with a callback.
     * When the Defer goes out of scope, then the callback is run.
     *
     * ```
     * bool resource_cleaned_up = false;
     * {
     *     // do something with resource that may throw an exception
     *     Defer close_file([&]() { resource_cleaned_up = true; });
     * }
     * assert(resource_cleaned_up);
     * ```
     */
    Defer(F &&callback)
        : m_callback { std::forward<F>(callback) } { }

    ~Defer() {
        m_callback();
    }

private:
    F m_callback;
};

}
