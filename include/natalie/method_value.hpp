#pragma once

#include "natalie/class_value.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/string_value.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct MethodValue : Value {
    MethodValue(Env *env, ModuleValue *owner, SymbolValue *owner_name, bool owner_is_singleton, SymbolValue *name, Method *method)
        : Value { Value::Type::Method, env->Object()->const_fetch(env, "Method")->as_class() }
        , m_owner { owner }
        , m_owner_name { owner_name }
        , m_owner_is_singleton { owner_is_singleton }
        , m_name { name }
        , m_method { method } { }

    MethodValue(Env *env, ClassValue *klass)
        : Value { Value::Type::Method, klass } { }

    ModuleValue *owner() { return m_owner; }
    SymbolValue *name() { return m_name; }
    Method *method() { return m_method; }

    ValuePtr inspect(Env *env) {
        if (m_owner_is_singleton)
            return StringValue::sprintf(env, "#<Method: %s.%s(*)>", m_owner_name->c_str(), m_name->c_str());
        else
            return StringValue::sprintf(env, "#<Method: %s#%s(*)>", m_owner_name->c_str(), m_name->c_str());
    }

private:
    ModuleValue *m_owner { nullptr };
    SymbolValue *m_owner_name { nullptr };
    bool m_owner_is_singleton { false };
    SymbolValue *m_name { nullptr };
    Method *m_method { nullptr };
};
}
