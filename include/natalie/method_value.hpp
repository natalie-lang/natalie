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
    MethodValue(Env *env, SymbolValue *owner_name, bool owner_is_singleton, Method *method)
        : Value { Value::Type::Method, env->Object()->const_fetch(env, "Method")->as_class() }
        , m_owner_name { owner_name }
        , m_owner_is_singleton { owner_is_singleton }
        , m_method { method } { }

    MethodValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Method, klass } { }

    ModuleValue *owner() { return m_method->owner(); }
    SymbolValue *name(Env *env) { return SymbolValue::intern(env, m_method->name()); }
    Method *method() { return m_method; }

    ValuePtr inspect(Env *env) {
        if (m_owner_is_singleton)
            return StringValue::sprintf(env, "#<Method: %s.%s(*)>", m_owner_name->c_str(), m_method->name());
        else
            return StringValue::sprintf(env, "#<Method: %s#%s(*)>", m_owner_name->c_str(), m_method->name());
    }

private:
    SymbolValue *m_owner_name { nullptr };
    bool m_owner_is_singleton { false };
    Method *m_method { nullptr };
};
}
