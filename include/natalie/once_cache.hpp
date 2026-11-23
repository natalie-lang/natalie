#pragma once

#include "natalie/gc/cell.hpp"
#include "natalie/value.hpp"
#include <mutex>

namespace Natalie {

// Each compiled literal site owns a process-lifetime slot. The registry keeps
// its cache and cached Ruby value visible to the GC, including between calls.
class OnceCache : public Cell {
public:
    static OnceCache *get(OnceCache **);
    static void visit_roots(Visitor &);

    class Guard {
    public:
        Guard(Env *, OnceCache **);
        ~Guard();
        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;

        Optional<Value> value() const { return m_cache->m_value; }
        void set(Value);

    private:
        OnceCache *m_cache;
        bool m_completed { false };
    };

    virtual void visit_children(Visitor &visitor) const override {
        if (m_value)
            visitor.visit(m_value.value());
    }

private:
    OnceCache() { }
    std::recursive_mutex m_mutex;
    Optional<Value> m_value {};
};

}
