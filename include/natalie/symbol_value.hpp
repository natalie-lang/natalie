#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct SymbolValue : Value {
    using Value::Value;

    SymbolValue(Env *env)
        : Value { env, Value::Type::Symbol, NAT_OBJECT->const_get(env, "Symbol", true)->as_class() } { }

    SymbolValue(Env *env, const char *symbol)
        : Value { env, Value::Type::Symbol, NAT_OBJECT->const_get(env, "Symbol", true)->as_class() }
        , symbol { symbol } { }

    const char *symbol { nullptr };
};

}
