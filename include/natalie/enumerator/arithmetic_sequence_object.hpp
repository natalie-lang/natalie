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

    static ArithmeticSequenceObject *from_range(Value begin, Value end, Value step, bool exclude_end) {
        return new ArithmeticSequenceObject { Origin::Range, begin, end, step, exclude_end };
    }

    static ArithmeticSequenceObject *from_numeric(Value begin, Value end, Value step) {
        return new ArithmeticSequenceObject { Origin::Numeric, begin, end, step, false };
    }

    Value begin() const { return m_begin; }
    Value end() const { return m_end; }
    bool eq(Env *, Value);
    bool exclude_end() const { return m_exclude_end; }
    Value hash(Env *);
    bool has_step() { return m_step && !m_step->is_nil(); }
    Value inspect(Env *);
    Value last(Env *);
    Value step() {
        if (has_step())
            return m_step;
        return Value::integer(1);
    }

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
    enum class Origin {
        Range,
        Numeric
    };

    ArithmeticSequenceObject(Origin origin, Value begin, Value end, Value step, bool exclude_end)
        : ArithmeticSequenceObject {} {
        m_origin = origin;
        m_begin = begin;
        m_end = end;
        m_exclude_end = exclude_end;
        m_step = step;
    }

    Integer step_count(Env *env) {
        if (!m_step_count.present())
            m_step_count = calculate_step_count(env);
        return m_step_count.value();
    }
    Integer calculate_step_count(Env *);

    Origin m_origin;
    Value m_begin { nullptr };
    Value m_end { nullptr };
    Value m_step { nullptr };
    Optional<Integer> m_step_count {};
    bool m_exclude_end { false };
};
};
