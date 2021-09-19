#pragma once

#include "tm/defer.hpp"
#include "tm/hashmap.hpp"
#include <assert.h>

namespace TM {

class RecursionGuard {
public:
    RecursionGuard(void *instance)
        : m_instance { instance } {
        assert(m_instance);
    }

    template <typename F>
    auto run(F callback) {
        if (seen())
            return callback(true);

        mark();
        Defer on_close([&]() { clear(); });
        return callback(false);
    }

private:
    void *m_instance { nullptr };

    bool seen() {
        return s_did_run.get(m_instance) != nullptr;
    }

    void mark() {
        s_did_run.set(m_instance);
    }

    void clear() {
        s_did_run.remove(m_instance);
    }

    static inline TM::Hashmap<void *> s_did_run {};
};

class PairedRecursionGuard {
public:
    PairedRecursionGuard(void *instance, void *other_instance)
        : m_instance { instance }
        , m_other_instance { other_instance } {
        assert(m_instance);
        assert(m_other_instance);
    }

    template <typename F>
    auto run(F callback) {
        if (seen())
            return callback(true);

        mark();
        Defer on_close([&]() { clear(); });
        return callback(false);
    }

private:
    void *m_instance { nullptr };
    void *m_other_instance { nullptr };

    bool seen() {
        auto companions = s_did_run.get(m_instance);
        if (!companions)
            return false;
        return companions->get(m_other_instance) != nullptr;
    }

    void mark() {
        auto companions = s_did_run.get(m_instance);
        if (!companions) {
            companions = new TM::Hashmap<void *> {};
            s_did_run.put(m_instance, companions);
        }
        companions->set(m_other_instance);
        return;
    }

    void clear() {
        auto companions = s_did_run.get(m_instance);
        if (!companions) {
            s_did_run.remove(m_instance);
            return;
        }
        companions->remove(m_other_instance);
        if (companions->is_empty()) {
            delete companions;
            s_did_run.remove(m_instance);
        }
    }

    static inline TM::Hashmap<void *, TM::Hashmap<void *> *> s_did_run {};
};

}
