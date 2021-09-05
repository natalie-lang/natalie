#pragma once

#include <functional>

#include "natalie/hashmap.hpp"
#include "tm/defer.hpp"

namespace Natalie {
static Hashmap<void *> did_run;

template<typename ReturnType>
class RecursionGuard {
public:
    RecursionGuard(void *instance) 
        : m_instance { instance } {
    }

    ReturnType run(std::function<ReturnType(bool)> callback) {
        if (did_run.get(m_instance) != nullptr)
            return callback(true);

        mark();
        Defer on_close([&] () { clear(); });
        return callback(false);
    }

private:
    void *m_instance;

    void mark() {
        if (did_run.get(m_instance) == nullptr) {
            did_run.set(m_instance);
        }
    }

    void clear() {
        if (did_run.get(m_instance) != nullptr) {
            did_run.remove(m_instance);
        }
    }
};



}