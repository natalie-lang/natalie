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

    virtual void visit_children(Visitor &visitor) override final {
        visitor.visit(&m_env);
    }

private:
    Env m_env;
};

}
