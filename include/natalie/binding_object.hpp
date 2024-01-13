#pragma once

#include "natalie/env.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class BindingObject : public Object {
public:
    BindingObject(Env *env)
        : Object { Object::Type::Binding, GlobalEnv::the()->Binding() }
        , m_env { *env } { }

    Env *env() { return &m_env; }

    Value source_location() const;

private:
    Env m_env;
};

}
