#pragma once

#include "natalie/class_value.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/method.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class MethodValue : public Value {
public:
    MethodValue(ValuePtr object, Method *method)
        : Value { Value::Type::Method, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Method"))->as_class() }
        , m_object { object }
        , m_method { method } { }

    MethodValue(ClassValue *klass)
        : Value { Value::Type::Method, klass } { }

    ModuleValue *owner() { return m_method->owner(); }
    SymbolValue *name(Env *env) { return SymbolValue::intern(m_method->name()); }
    Method *method() { return m_method; }

    ValuePtr inspect(Env *env) {
        auto the_owner = owner();
        if (the_owner->is_class() && the_owner->as_class()->is_singleton())
            return StringValue::format(env, "#<Method: {}.{}(*)>", m_object->inspect_str(env), m_method->name());
        else
            return StringValue::format(env, "#<Method: {}#{}(*)>", owner()->class_name_or_blank(), m_method->name());
    }

    int arity() { return m_method ? m_method->arity() : 0; }

    virtual void visit_children(Visitor &visitor) override final {
        Value::visit_children(visitor);
        visitor.visit(m_object);
        visitor.visit(m_method);
    }

    virtual void gc_print() override {
        if (m_method) {
            fprintf(stderr, "<MethodValue %p method=", this);
            m_method->gc_print();
            fprintf(stderr, ">");
        } else {
            fprintf(stderr, "<MethodValue %p method=null>", this);
        }
    }

private:
    ValuePtr m_object { nullptr };
    Method *m_method { nullptr };
};
}
