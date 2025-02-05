#pragma once

#include "natalie/forward.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class RationalObject : public Object {
public:
    RationalObject(Value numerator, Value denominator)
        : Object { Object::Type::Rational, GlobalEnv::the()->Object()->const_fetch("Rational"_s).as_class() }
        , m_numerator { numerator.integer() }
        , m_denominator { denominator.integer() } {
        freeze();
    }

    RationalObject(const RationalObject &other)
        : Object { Object::Type::Rational, GlobalEnv::the()->Object()->const_fetch("Rational"_s).as_class() }
        , m_numerator { other.m_numerator }
        , m_denominator { other.m_denominator } {
        freeze();
    }

    static RationalObject *create(Env *env, Integer numerator, Integer denominator);

    bool is_zero() const {
        return m_numerator.is_zero();
    }

    Value add(Env *, Value);
    Value cmp(Env *, Value);
    Value coerce(Env *, Value);
    Value denominator(Env *);
    Value div(Env *, Value);
    bool eq(Env *, Value);
    Value floor(Env *, Value);
    Value inspect(Env *);
    Value marshal_dump(Env *);
    Value mul(Env *, Value);
    Value numerator(Env *);
    Value pow(Env *, Value);
    Value rationalize(Env *);
    Value sub(Env *, Value);
    Value to_f(Env *);
    Value to_i(Env *);
    Value to_r() { return this; }
    Value to_s(Env *);
    Value truncate(Env *, Value);

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_numerator);
        visitor.visit(m_denominator);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<RationalObject %p>", this);
    }

private:
    Integer m_numerator;
    Integer m_denominator;
};

}
