#pragma once

#include "natalie/hashmap.hpp" // this will be moved into TM namespace someday real soon :-)
#include "tm/defer.hpp"

namespace Natalie {

class RecursionGuard {
public:
    RecursionGuard(void *instance)
        : m_instance { instance } {
    }

    template <typename F>
    auto run(F callback) {
        if (s_did_run.get(m_instance) != nullptr)
            return callback(true);

        mark();
        Defer on_close([&]() { clear(); });
        return callback(false);
    }

private:
    void *m_instance;

    void mark() {
        s_did_run.set(m_instance);
    }

    void clear() {
        s_did_run.remove(m_instance);
    }

    static inline Hashmap<void *> s_did_run {};
};

}
