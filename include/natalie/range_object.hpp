#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class RangeObject : public Object {
public:
    RangeObject()
        : Object { Object::Type::Range, GlobalEnv::the()->Object()->const_fetch("Range"_s).as_class() } { }

    RangeObject(ClassObject *klass)
        : Object { Object::Type::Range, klass } { }

    static RangeObject *create(Env *, Value, Value, bool);

    Value begin() { return m_begin; }
    Value end() { return m_end; }
    bool exclude_end() const { return m_exclude_end; }

    Value initialize(Env *, Value, Value, Optional<Value>);
    Value to_a(Env *);
    Value each(Env *, Block *);
    Value first(Env *, Optional<Value>);
    Value inspect(Env *);
    Value last(Env *, Optional<Value>);
    String to_s() const;
    Value to_s(Env *);
    bool eq(Env *, Value);
    bool eql(Env *, Value);
    bool include(Env *, Value);
    Value bsearch(Env *, Block *);
    Value step(Env *, Optional<Value>, Block *);

    static Value size_fn(Env *env, Value self, Args &&, Block *) {
        return Value::integer(self.as_range()->to_a(env).as_array()->size());
    }

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_begin);
        visitor.visit(m_end);
    }

    virtual String dbg_inspect() const override;

private:
    RangeObject(Value begin, Value end, bool exclude_end)
        : Object { Object::Type::Range, GlobalEnv::the()->Object()->const_fetch("Range"_s).as_class() }
        , m_begin { begin }
        , m_end { end }
        , m_exclude_end { exclude_end } { }

    static void assert_no_bad_value(Env *, Value, Value);

    Value m_begin { Value::nil() };
    Value m_end { Value::nil() };
    bool m_exclude_end { false };

    template <typename Function>
    Optional<Value> iterate_over_range(Env *env, Function &&f);

    template <typename Function>
    Optional<Value> iterate_over_integer_range(Env *env, Function &&f);

    template <typename Function>
    Optional<Value> iterate_over_string_range(Env *env, Function &&f);

    template <typename Function>
    Optional<Value> iterate_over_symbol_range(Env *env, Function &&f);
};

}
