#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/thread_object.hpp"

#include <condition_variable>

namespace Natalie {

class Thread::ConditionVariableObject : public Object {
public:
    ConditionVariableObject()
        : Object { Object::Type::ThreadConditionVariable, GlobalEnv::the()->Object()->const_fetch("Thread"_s)->const_fetch("ConditionVariable"_s)->as_class() } { }

    ConditionVariableObject(ClassObject *klass)
        : Object { Object::Type::ThreadConditionVariable, klass } { }

    Value signal(Env *);
    Value wait(Env *, Value, Value = nullptr);

private:
    std::condition_variable m_cv {};
};

}
