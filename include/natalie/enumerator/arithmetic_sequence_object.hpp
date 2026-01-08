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
        : ArithmeticSequenceObject { GlobalEnv::the()->Object()->const_fetch("Enumerator"_s).as_class()->const_fetch("ArithmeticSequence"_s).as_class() } { }

    static ArithmeticSequenceObject *from_range(Env *env, const TM::String &origin_method, Value begin, Value end, Value step, bool exclude_end) {
        return new ArithmeticSequenceObject { env, Origin::Range, origin_method, begin, end, step, exclude_end };
    }

    static ArithmeticSequenceObject *from_numeric(Env *env, Value begin, Value end, Value step) {
        return new ArithmeticSequenceObject { env, Origin::Numeric, begin, end, step, false };
    }

    Value begin() const { return m_begin; }
    Value each(Env *, Block *);
    Value end() const { return m_end; }
    bool eq(Env *, Value);
    bool exclude_end() const { return m_exclude_end; }
    Value hash(Env *);
    bool has_step() { return !m_step.is_nil(); }
    Value inspect(Env *);
    Value last(Env *, Optional<Value>);
    Value size(Env *);
    Value step() {
        if (has_step())
            return m_step;
        return Value::integer(1);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<Enumerator::ArithmeticSequence {h}>", this);
    }

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_begin);
        visitor.visit(m_end);
        visitor.visit(m_step);
        if (m_step_count)
            visitor.visit(m_step_count.value());
    }

private:
    enum class Origin {
        Range,
        Numeric
    };

    ArithmeticSequenceObject(Env *, Origin, Value, Value, Value, bool);
    ArithmeticSequenceObject(Env *, Origin, const TM::String &, Value, Value, Value, bool);
    static Value enum_block(Env *, Value, Args &&, Block *);

    bool ascending(Env *env) {
        if (!m_ascending.has_value())
            m_ascending = calculate_ascending(env);
        return m_ascending.value();
    }
    bool calculate_ascending(Env *);
    Integer calculate_step_count(Env *);
    Value iterate(Env *, std::function<Value(Value)>);
    Integer step_count(Env *env) {
        if (!m_step_count.has_value())
            m_step_count = calculate_step_count(env);
        return m_step_count.value();
    }
    Value maybe_to_f(Env *, Value);

    Origin m_origin;
    TM::String m_range_origin_method {};
    Value m_begin {};
    Value m_end {};
    Value m_step {};
    Optional<Integer> m_step_count {};
    bool m_exclude_end { false };
    Optional<bool> m_ascending {};
};
};
