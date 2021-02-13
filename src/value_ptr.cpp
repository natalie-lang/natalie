#include "natalie.hpp"

namespace Natalie {

ValuePtr::ValuePtr(Env *env, nat_int_t integer)
    : m_type { Type::Integer }
    , m_global_env { env->global_env() }
    , m_integer { integer } { }

ValuePtr ValuePtr::send(Env *env, SymbolValue *name, size_t argc, ValuePtr *args, Block *block) {
    if (m_type == Type::Integer && IntegerValue::optimized_method(name)) {
        auto synthesized = IntegerValue { m_global_env, m_integer };
        return synthesized._send(env, name, argc, args, block);
    }

    return value()->_send(env, name, argc, args, block);
}

ValuePtr ValuePtr::send(Env *env, const char *name, size_t argc, ValuePtr *args, Block *block) {
    return value()->_send(env, name, argc, args, block);
}

ValuePtr ValuePtr::send(Env *env, size_t argc, ValuePtr *args, Block *block) {
    return value()->_send(env, argc, args, block);
}

void ValuePtr::hydrate() {
    switch (m_type) {
    case Type::Integer:
        m_type = Type::Pointer;
        m_value = new IntegerValue { m_global_env, m_integer };
        m_integer = 0;
        break;
    case Type::Pointer:
        break;
    }
}

bool ValuePtr::is_float() {
    if (m_type == Type::Integer)
        return false;

    return value()->is_float();
}

void ValuePtr::assert_type(Env *env, ValueType type, const char *type_name) {
    if (m_type == Type::Integer && type == ValueType::Integer)
        return;

    value()->assert_type(env, type, type_name);
}

nat_int_t ValuePtr::to_nat_int_t() {
    if (m_type == Type::Integer)
        return m_integer;

    return value()->as_integer()->to_nat_int_t();
}

}
