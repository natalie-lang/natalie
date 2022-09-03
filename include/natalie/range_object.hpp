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
        : Object { Object::Type::Range, GlobalEnv::the()->Object()->const_fetch("Range"_s)->as_class() } { }

    RangeObject(ClassObject *klass)
        : Object { Object::Type::Range, klass } { }

    RangeObject(Value begin, Value end, bool exclude_end)
        : Object { Object::Type::Range, GlobalEnv::the()->Object()->const_fetch("Range"_s)->as_class() }
        , m_begin { begin }
        , m_end { end }
        , m_exclude_end { exclude_end } {
        freeze();
    }

    Value begin() { return m_begin; }
    Value end() { return m_end; }
    bool exclude_end() const { return m_exclude_end; }

    Value initialize(Env *, Value, Value, Value);
    Value to_a(Env *);
    Value each(Env *, Block *);
    Value first(Env *, Value);
    Value inspect(Env *);
    Value last(Env *, Value);
    Value to_s(Env *);
    bool eq(Env *, Value);
    bool eql(Env *, Value);
    bool include(Env *, Value);
    Value bsearch(Env *, Block *);

    static Value size_fn(Env *env, Value self, Args, Block *) {
        return Value::integer(self->as_range()->to_a(env)->as_array()->size());
    }

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_begin);
        visitor.visit(m_end);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<RangeObject %p>", this);
    }

private:
    Value m_begin { nullptr };
    Value m_end { nullptr };
    bool m_exclude_end { false };

    template <typename Function>
    Value iterate_over_range(Env *env, Function &&f);

    template <typename Function>
    Value iterate_over_string_range(Env *env, Function &&f);

    template <typename Function>
    Value iterate_over_symbol_range(Env *env, Function &&f);
};

}
