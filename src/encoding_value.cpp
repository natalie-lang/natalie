#include "natalie.hpp"

namespace Natalie {

EncodingValue::EncodingValue(Env *env)
    : Value { Value::Type::Encoding, env->Object()->const_fetch(env, SymbolValue::intern(env, "Encoding"))->as_class() } { }

ArrayValue *EncodingValue::list(Env *env) {
    ArrayValue *ary = new ArrayValue { env };
    ValuePtr Encoding = env->Object()->const_fetch(env, SymbolValue::intern(env, "Encoding"));
    ary->push(Encoding->const_fetch(env, SymbolValue::intern(env, "ASCII_8BIT")));
    ary->push(Encoding->const_fetch(env, SymbolValue::intern(env, "UTF_8")));
    return ary;
}

EncodingValue::EncodingValue(Env *env, Encoding num, std::initializer_list<const char *> names)
    : EncodingValue { env } {
    m_num = num;
    for (const char *name : names) {
        m_names.push(new StringValue { env, name });
    }
}

ValuePtr EncodingValue::name(Env *env) {
    return m_names[0]->dup(env);
}

ArrayValue *EncodingValue::names(Env *env) {
    auto array = new ArrayValue { env };
    for (StringValue *name : m_names) {
        array->push(name);
    }
    return array;
}

ValuePtr EncodingValue::inspect(Env *env) {
    return StringValue::format(env, "#<Encoding:{}>", name());
}

}
