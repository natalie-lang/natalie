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
    static ComplexObject *create() {
        return new ComplexObject();
    }

    static ComplexObject *create(ClassObject *klass) {
        return new ComplexObject(klass);
    }

    static ComplexObject *create(Value real) {
        return new ComplexObject(real);
    }

    static ComplexObject *create(Value real, Value imaginary) {
        return new ComplexObject(real, imaginary);
    }

    static ComplexObject *create(const ComplexObject &other) {
        return new ComplexObject(other);
    }

    Value imaginary() { return m_imaginary; }
    Value inspect(Env *);
    Value real() { return m_real; }

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_real);
        visitor.visit(m_imaginary);
    }

protected:
    ComplexObject()
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s).as_class() } {
        freeze();
    }

    ComplexObject(ClassObject *klass)
        : Object { Object::Type::Complex, klass } {
        freeze();
    }

    ComplexObject(Value real)
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s).as_class() }
        , m_real { real }
        , m_imaginary { Value::integer(0) } {
        freeze();
    }

    ComplexObject(Value real, Value imaginary)
        : Object { Object::Type::Complex, GlobalEnv::the()->Object()->const_fetch("Complex"_s).as_class() }
        , m_real { real }
        , m_imaginary { imaginary } {
        freeze();
    }

    ComplexObject(const ComplexObject &other)
        : Object { other }
        , m_real { other.m_real }
        , m_imaginary { other.m_imaginary } {
        freeze();
    }

private:
    Value m_real {};
    Value m_imaginary {};
};
}
