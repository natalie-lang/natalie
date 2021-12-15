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
        : Object { Object::Type::Range, GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Range"))->as_class() } { }

    RangeObject(ClassObject *klass)
        : Object { Object::Type::Range, klass } { }

    RangeObject(ValuePtr begin, ValuePtr end, bool exclude_end)
        : Object { Object::Type::Range, GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Range"))->as_class() }
        , m_begin { begin }
        , m_end { end }
        , m_exclude_end { exclude_end } { }

    ValuePtr begin() { return m_begin; }
    ValuePtr end() { return m_end; }
    bool exclude_end() const { return m_exclude_end; }

    ValuePtr initialize(Env *, ValuePtr, ValuePtr, ValuePtr);
    ValuePtr to_a(Env *);
    ValuePtr each(Env *, Block *);
    ValuePtr first(Env *, ValuePtr);
    ValuePtr inspect(Env *);
    ValuePtr eq(Env *, ValuePtr);
    ValuePtr eqeqeq(Env *, ValuePtr);
    ValuePtr include(Env *, ValuePtr);

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_begin);
        visitor.visit(m_end);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<RangeObject %p>", this);
    }

private:
    ValuePtr m_begin { nullptr };
    ValuePtr m_end { nullptr };
    bool m_exclude_end { false };

    template <typename Function>
    ValuePtr iterate_over_range(Env *env, Function &&f);
};

}
