#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/encoding_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct StringValue : Value {
    using Value::Value;

    StringValue(Env *env)
        : Value { env, Value::Type::String, NAT_OBJECT->const_get(env, "String", true)->as_class() } { }

    StringValue(Env *env, ClassValue *klass)
        : Value { env, Value::Type::String, klass } { }

    ssize_t str_len { 0 };
    ssize_t str_cap { 0 };
    char *str { nullptr };
    Encoding encoding;

    SymbolValue *to_symbol(Env *env);
};

}
