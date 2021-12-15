#include "natalie.hpp"

namespace Natalie {

ValuePtr ValuePtr::public_send(Env *env, SymbolValue *name, size_t argc, ValuePtr *args, Block *block) {
    if (m_type == Type::Integer && IntegerValue::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerValue { m_integer };
        return synthesized.public_send(env, name, argc, args, block);
    }

    return value()->public_send(env, name, argc, args, block);
}

ValuePtr ValuePtr::send(Env *env, SymbolValue *name, size_t argc, ValuePtr *args, Block *block) {
    if (m_type == Type::Integer && IntegerValue::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerValue { m_integer };
        return synthesized.send(env, name, argc, args, block);
    }

    return value()->send(env, name, argc, args, block);
}

void ValuePtr::hydrate() {
    switch (m_type) {
    case Type::Integer: {
        m_type = Type::Pointer;
        m_value = new IntegerValue { m_integer };
        m_integer = 0;
        break;
    }
    case Type::Pointer:
        break;
    }
}

}
