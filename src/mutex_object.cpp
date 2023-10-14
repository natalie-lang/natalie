#include "natalie.hpp"

namespace Natalie {

Value MutexObject::lock(Env *env) {
    const std::lock_guard lock(m_mutex);
    if (m_owner) {
        if (m_owner == FiberObject::current())
            env->raise("ThreadError", "deadlock; recursive locking");
        // NATFIXME: Recursive search in fiber parents
        // NATFIXME: Block
        // NATFIXME: Call fiber scheduler
    }
    m_owner = FiberObject::current();
    return this;
}

Value MutexObject::unlock(Env *env) {
    const std::lock_guard lock(m_mutex);
    if (!m_owner)
        env->raise("ThreadError", "Attempt to unlock a mutex which is not locked");
    // NATFIXME: Validate owner
    m_owner = nullptr;
    return this;
}

void MutexObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_owner);
}

}
