#pragma once

#include "tm/defer.hpp"
#include "tm/hashmap.hpp"

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

    static inline TM::Hashmap<void *> s_did_run {};
};

}
