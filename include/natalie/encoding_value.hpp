#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

enum class Encoding {
    ASCII_8BIT = 1,
    UTF_8 = 2,
};

struct EncodingValue : Value {

    using Value::Value;

    EncodingValue(Env *env)
        : Value { env, Value::Type::Encoding, const_get(env, NAT_OBJECT, "Encoding", true)->as_class() } { }

    ArrayValue *encoding_names { nullptr };
    Encoding encoding_num;
};

EncodingValue *encoding(Env *env, Encoding num, ArrayValue *names);

}
