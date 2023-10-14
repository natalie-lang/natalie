#pragma once

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class MutexObject : public Object {
public:
    MutexObject()
        : Object { Object::Type::Mutex, GlobalEnv::the()->Object()->const_fetch("Mutex"_s)->as_class() } { }

    MutexObject(ClassObject *klass)
        : Object { Object::Type::Mutex, klass } { }

    Value lock(Env *);
    Value unlock(Env *);
};

}
