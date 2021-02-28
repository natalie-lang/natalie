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

struct MethodValue : Value {
    MethodValue(Env *env, ValuePtr object, Method *method)
        : Value { Value::Type::Method, env->Object()->const_fetch(env, SymbolValue::intern(env, "Method"))->as_class() }
        , m_object { object }
        , m_method { method } { }

    MethodValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Method, klass } { }

    ModuleValue *owner() { return m_method->owner(); }
    SymbolValue *name(Env *env) { return SymbolValue::intern(env, m_method->name()); }
    Method *method() { return m_method; }

    ValuePtr inspect(Env *env) {
        auto the_owner = owner();
        if (the_owner->is_class() && the_owner->as_class()->is_singleton())
            return StringValue::sprintf(env, "#<Method: %s.%s(*)>", m_object->inspect_str(env), m_method->name());
        else
            return StringValue::sprintf(env, "#<Method: %s#%s(*)>", owner()->class_name(), m_method->name());
    }

    int arity() { return m_method ? m_method->arity() : 0; }

private:
    ValuePtr m_object { nullptr };
    Method *m_method { nullptr };
};
}
