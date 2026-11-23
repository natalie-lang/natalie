#include "natalie/once_cache.hpp"
#include "natalie.hpp"

namespace Natalie {

static Vector<OnceCache *> s_once_caches;

OnceCache *OnceCache::get(OnceCache **slot) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    if (!*slot) {
        auto cache = new OnceCache;
        s_once_caches.push(cache);
        *slot = cache;
    }
    return *slot;
}

void OnceCache::visit_roots(Visitor &visitor) {
    for (auto cache : s_once_caches)
        visitor.visit(cache);
}

OnceCache::Guard::Guard(Env *env, OnceCache **slot)
    : m_cache { OnceCache::get(slot) } {
    if (!m_cache->m_mutex.try_lock()) {
        Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
        ThreadObject::set_current_sleeping(true);
        struct timespec request = { 0, 100000 };
        while (!m_cache->m_mutex.try_lock()) {
            ThreadObject::deliver_current_pending(env, ThreadObject::CheckpointKind::Blocking);
            nanosleep(&request, nullptr);
        }
    }
    m_completed = m_cache->m_value.has_value();
}

OnceCache::Guard::~Guard() {
    // An outer recursive evaluation that unwinds must discard any value
    // produced by an inner evaluation, just as an ordinary failed attempt does.
    if (!m_completed) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        m_cache->m_value = Optional<Value> {};
    }
    m_cache->m_mutex.unlock();
}

void OnceCache::Guard::set(Value value) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
    m_cache->m_value = value;
    m_completed = true;
}

}
