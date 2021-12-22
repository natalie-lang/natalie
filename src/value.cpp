#include "natalie.hpp"

namespace Natalie {

Value Value::public_send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        return synthesized.public_send(env, name, argc, args, block);
    }

    return object()->public_send(env, name, argc, args, block);
}

Value Value::send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        return synthesized.send(env, name, argc, args, block);
    }

    return object()->send(env, name, argc, args, block);
}

void Value::hydrate() {
    switch (m_type) {
    case Type::Integer: {
        // Running GC while we're in the processes of hydrating this Value makes
        // debugging VERY confusing. Maybe someday we can remove this GC stuff...
        bool was_gc_enabled = Heap::the().gc_enabled();
        Heap::the().gc_disable();
        m_type = Type::Pointer;
        m_object = new IntegerObject { m_integer };
        m_integer = 0;
        if (was_gc_enabled) Heap::the().gc_enable();
        break;
    }
    case Type::Pointer:
        break;
    }
}

}
