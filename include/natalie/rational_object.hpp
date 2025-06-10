#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class RationalObject : public Object {
public:
    static RationalObject *create(Value numerator, Value denominator) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new RationalObject { numerator, denominator };
    }

    static RationalObject *create(const RationalObject &other) {
        std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);
        return new RationalObject { other };
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
    Value floor(Env *, Optional<Value> = {});
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
    Value truncate(Env *, Optional<Value>);

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_numerator);
        visitor.visit(m_denominator);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<RationalObject {h} {}/{}>", this, m_numerator.to_string(), m_denominator.to_string());
    }

private:
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

    Integer m_numerator;
    Integer m_denominator;
};

}
