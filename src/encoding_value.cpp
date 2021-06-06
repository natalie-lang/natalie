#include "natalie.hpp"

namespace Natalie {

EncodingValue::EncodingValue(Env *env)
    : Value { Value::Type::Encoding, env->Object()->const_fetch(SymbolValue::intern("Encoding"))->as_class() } { }

ArrayValue *EncodingValue::list(Env *env) {
    ArrayValue *ary = new ArrayValue {};
    ValuePtr Encoding = env->Object()->const_fetch(SymbolValue::intern("Encoding"));
    ary->push(Encoding->const_fetch(SymbolValue::intern("ASCII_8BIT")));
    ary->push(Encoding->const_fetch(SymbolValue::intern("UTF_8")));
    return ary;
}

EncodingValue::EncodingValue(Env *env, Encoding num, std::initializer_list<const char *> names)
    : EncodingValue { env } {
    m_num = num;
    for (const char *name : names) {
        m_names.push(new StringValue { name });
    }
}

ValuePtr EncodingValue::name(Env *env) {
    return m_names[0]->dup(env);
}

ArrayValue *EncodingValue::names(Env *env) {
    auto array = new ArrayValue {};
    for (StringValue *name : m_names) {
        array->push(name);
    }
    return array;
}

ValuePtr EncodingValue::inspect(Env *env) {
    return StringValue::format(env, "#<Encoding:{}>", name());
}

void EncodingValue::visit_children(Visitor &visitor) {
    Value::visit_children(visitor);
    for (auto name : m_names) {
        visitor.visit(name);
    }
}

}
