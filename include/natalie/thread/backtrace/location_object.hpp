#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"
#include "natalie/thread_object.hpp"

namespace Natalie {

class Thread::Backtrace::LocationObject : public Object {
public:
    LocationObject()
        : Object { Object::Type::ThreadBacktraceLocation, GlobalEnv::the()->Object()->const_fetch("Thread"_s)->const_fetch("Backtrace"_s)->const_fetch("Location"_s)->as_class() } { }

    LocationObject(ClassObject *klass)
        : Object { Object::Type::ThreadBacktraceLocation, klass } { }
};

}
