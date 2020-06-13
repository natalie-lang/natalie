#include "natalie.hpp"

namespace Natalie {

EncodingValue::EncodingValue(Env *env, Encoding num, std::initializer_list<const char *> names)
    : EncodingValue { env } {
    m_num = num;
    for (const char *name : names) {
        m_names.push(new StringValue { env, name });
    }
}

ArrayValue *EncodingValue::names(Env *env) {
    auto array = new ArrayValue { env };
    for (StringValue *name : m_names) {
        array->push(name);
    }
    return array;
}
}
