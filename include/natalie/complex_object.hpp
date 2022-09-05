#pragma once

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class ComplexObject : public Object {

public:
    ComplexObject()
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s)->as_class() } { }

    ComplexObject(ClassObject *klass)
        : Object { Object::Type::Complex, klass } { }

    ComplexObject(Value real)
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s)->as_class() }
        , m_real { real }
        , m_imaginary { Value::integer(0) } { }

    ComplexObject(Value real, Value imaginary)
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s)->as_class() }
        , m_real { real }
        , m_imaginary { imaginary } { }

    Value inspect(Env *);

private:
    Value m_real { nullptr };
    Value m_imaginary { nullptr };
};
}
