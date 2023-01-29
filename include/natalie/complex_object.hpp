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
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s)->as_class() } {
        freeze();
    }

    ComplexObject(ClassObject *klass)
        : Object { Object::Type::Complex, klass } {
        freeze();
    }

    ComplexObject(Value real)
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s)->as_class() }
        , m_real { real }
        , m_imaginary { Value::integer(0) } {
        freeze();
    }

    ComplexObject(Value real, Value imaginary)
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s)->as_class() }
        , m_real { real }
        , m_imaginary { imaginary } {
        freeze();
    }

    Value imaginary(Env *);
    Value inspect(Env *);
    Value real(Env *);

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_real);
        visitor.visit(m_imaginary);
    }

private:
    Value m_real { nullptr };
    Value m_imaginary { nullptr };
};
}
