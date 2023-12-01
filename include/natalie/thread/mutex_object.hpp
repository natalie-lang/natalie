#pragma once

#include "natalie/fiber_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/thread_object.hpp"

#include <mutex>

namespace Natalie {

class Thread::MutexObject : public Object {
public:
    MutexObject()
        : Object { Object::Type::ThreadMutex, GlobalEnv::the()->Object()->const_fetch("Thread"_s)->const_fetch("Mutex"_s)->as_class() } { }

    MutexObject(ClassObject *klass)
        : Object { Object::Type::ThreadMutex, klass } { }

    Value lock(Env *);
    Value synchronize(Env *, Block *);
    bool try_lock();
    Value unlock(Env *);

    void unlock_without_checks() { m_mutex.unlock(); }

    bool is_locked();
    bool is_owned();

    void visit_children(Visitor &);

private:
    std::mutex m_mutex;
    ThreadObject *m_thread { nullptr };
    FiberObject *m_fiber { nullptr };
};

}
