#include "natalie.hpp"

namespace Natalie {

Value Value::public_send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        return synthesized.public_send(env, name, argc, args, block);
    }

    return value()->public_send(env, name, argc, args, block);
}

Value Value::send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        return synthesized.send(env, name, argc, args, block);
    }

    return value()->send(env, name, argc, args, block);
}

void Value::hydrate() {
    switch (m_type) {
    case Type::Integer: {
        m_type = Type::Pointer;
        m_value = new IntegerObject { m_integer };
        m_integer = 0;
        break;
    }
    case Type::Pointer:
        break;
    }
}

}
