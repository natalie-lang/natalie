#pragma once

#include <mutex>

#include "natalie/array_object.hpp"
#include "natalie/class_object.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/thread_object.hpp"

namespace Natalie {

class ThreadGroupObject : public Object {
public:
    ThreadGroupObject()
        : Object { Object::Type::ThreadGroup, GlobalEnv::the()->Object()->const_fetch("ThreadGroup"_s)->as_class() } { }

    ThreadGroupObject(ClassObject *klass)
        : Object { Object::Type::ThreadGroup, klass } { }

    ThreadGroupObject(ClassObject *klass, Block *block)
        : Object { Object::Type::ThreadGroup, klass } { }

    Value add(Env *, Value);
    Value enclose();
    bool is_enclosed() const { return m_enclosed; }
    ArrayObject *list();

    static void initialize_default();
    static ThreadGroupObject *get_default() { return m_default; }

    virtual void visit_children(Visitor &) override final;

private:
    std::mutex m_mutex;

    static inline ThreadGroupObject *m_default { nullptr };
    Vector<ThreadObject *> m_threads;
    bool m_enclosed { false };
};

}
