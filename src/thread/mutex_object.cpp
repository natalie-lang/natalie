#include "natalie.hpp"

namespace Natalie::Thread {

std::mutex g_mutex_mutex; // That's a funny name. :-)

Value MutexObject::lock(Env *env) {
    auto locked = m_mutex.try_lock();

    if (!locked) {
        if (ThreadObject::current() == m_thread) {
            env->raise("ThreadError", "deadlock; recursive locking");
        } else {
            Defer done_sleeping([] { ThreadObject::set_current_sleeping(false); });
            ThreadObject::set_current_sleeping(true);
            struct timespec request = { 0, 100000 };
            while (!m_mutex.try_lock())
                nanosleep(&request, nullptr);
        }
    }

    std::lock_guard<std::mutex> lock(g_mutex_mutex);
    m_thread = ThreadObject::current();
    m_thread->add_mutex(this);
    m_fiber = FiberObject::current();

    return this;
}

Value MutexObject::sleep(Env *env, Value timeout) {
    if (!timeout) {
        unlock(env);
        while (true)
            ::sleep(1000);
        lock(env);
        return this;
    }

    if ((timeout->is_float() && timeout->as_float()->is_negative()) || (timeout->is_integer() && timeout->as_integer()->is_negative()))
        env->raise("ArgumentError", "time interval must not be negative");

    auto timeout_int = IntegerObject::convert_to_nat_int_t(env, timeout);

    if (timeout_int < 0)
        env->raise("ArgumentError", "timeout must be positive");

    unlock(env);
    ::sleep(timeout_int);
    lock(env);

    return Value::integer(timeout_int);
}

Value MutexObject::synchronize(Env *env, Block *block) {
    try {
        lock(env);
        auto value = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, {}, nullptr);
        unlock(env);
        return value;
    } catch (ExceptionObject *exception) {
        unlock(env);
        throw exception;
    }
}

bool MutexObject::try_lock() {
    return m_mutex.try_lock();
}

Value MutexObject::unlock(Env *env) {
    std::lock_guard<std::mutex> lock(g_mutex_mutex);

    if (!is_locked())
        env->raise("ThreadError", "Attempt to unlock a mutex which is not locked");

    if (m_thread->status(env)->is_falsey())
        env->raise("ThreadError", "Attempt to unlock a mutex which is not locked");

    if (m_thread && m_thread != ThreadObject::current())
        env->raise("ThreadError", "Attempt to unlock a mutex which is locked by another thread/fiber");

    m_mutex.unlock();
    assert(m_thread);
    m_thread->remove_mutex(this);
    m_thread = nullptr;
    m_fiber = nullptr;
    return this;
}

bool MutexObject::is_locked() {
    auto now_locked = m_mutex.try_lock();
    if (now_locked) {
        m_mutex.unlock();
        return false;
    }

    return true;
}

bool MutexObject::is_owned() {
    std::lock_guard<std::mutex> lock(g_mutex_mutex);

    if (!is_locked()) return false;

    if (m_thread != ThreadObject::current())
        return false;

    if (m_fiber != FiberObject::current())
        return false;

    return true;
}

void MutexObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    visitor.visit(m_thread);
    visitor.visit(m_fiber);
}

}
