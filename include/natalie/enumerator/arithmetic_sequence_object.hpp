#pragma once

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/integer.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {
class Enumerator::ArithmeticSequenceObject : public Object {
public:
    ArithmeticSequenceObject(ClassObject *klass)
        : Object { Object::Type::EnumeratorArithmeticSequence, klass } { }

    ArithmeticSequenceObject()
        : ArithmeticSequenceObject { GlobalEnv::the()->Object()->const_fetch("Enumerator"_s)->const_fetch("ArithmeticSequence"_s)->as_class() } { }

    ArithmeticSequenceObject(Value begin, Value end, Value step, bool exclude_end)
        : ArithmeticSequenceObject {} {
        m_begin = begin;
        m_end = end;
        m_exclude_end = exclude_end;

        if (!step)
            step = Value::integer(1);
        m_step = step;
    }

    Value begin() const { return m_begin; }
    Value end() const { return m_end; }
    bool eq(Env *, Value);
    bool exclude_end() const { return m_exclude_end; }
    Value last(Env *);
    Value step() const { return m_step; }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Enumerator::ArithmeticSequence %p>", this);
    }

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_begin);
        visitor.visit(m_end);
        visitor.visit(m_step);
    }

private:
    Integer step_count(Env *env) {
        if (!m_step_count.present())
            m_step_count = calculate_step_count(env);
        return m_step_count.value();
    }
    Integer calculate_step_count(Env *);

    Value m_begin { nullptr };
    Value m_end { nullptr };
    Value m_step { nullptr };
    Optional<Integer> m_step_count {};
    bool m_exclude_end { false };
};
};
