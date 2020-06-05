#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct RegexpValue : Value {
    using Value::Value;

    RegexpValue(Env *env)
        : Value { env, Value::Type::Regexp, const_get(env, NAT_OBJECT, "Regexp", true)->as_class() } { }

    regex_t *regexp { nullptr };
    char *regexp_str { nullptr };
};

}
