#pragma once

#include "natalie/env.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class BindingValue : public Value {
public:
    BindingValue(Env *env)
        : Value { Value::Type::Binding, GlobalEnv::the()->Binding() }
        , m_env { *env } { }

    Env *env() { return &m_env; }

private:
    Env m_env;
};

}
