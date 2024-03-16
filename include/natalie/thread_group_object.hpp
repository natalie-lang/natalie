#pragma once

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

    static void initialize_default();
    static ThreadGroupObject *get_default() { return m_default; }

    virtual void visit_children(Visitor &) override final;

private:
    static inline ThreadGroupObject *m_default { nullptr };
};

}
