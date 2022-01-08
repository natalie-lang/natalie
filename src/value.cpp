#include "natalie.hpp"
#include <chrono>
#include <ctime>

namespace Natalie {

#define PROFILED_SEND(category)                                                            \
    auto classnameOf = [](Value val) {                                                     \
        return (val.m_type == Type::Integer) ? "FastInteger"                               \
            : (val.m_type == Type::Double)   ? "FastDouble"                                \
                                             : val->klass()->class_name().value()->c_str(); \
    };                                                                                     \
    auto type = classnameOf(*this);                                                        \
    auto event_name = new String();                                                        \
    event_name->append_sprintf("%s.%s(", type, name->c_str());                             \
    for (size_t i = 0; i < argc; ++i) {                                                    \
        if (i > 0)                                                                         \
            event_name->append_char(',');                                                  \
        event_name->append(classnameOf(args[i]));                                          \
    }                                                                                      \
    event_name->append_char(')');                                                          \
    auto event = NativeProfilerEvent::named(event_name)                                    \
                     ->tid(gettid())                                                       \
                     ->start(std::chrono::system_clock::now());                            \
    Defer logEvent([&]() {                                                                 \
        auto source_filename = env->file();                                                \
        auto source_line = env->line();                                                    \
        if (NativeProfiler::the()->enabled())                                              \
            NativeProfiler::the()->push(event->end(std::chrono::system_clock::now()));     \
    });

Value Value::public_send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    PROFILED_SEND(public_send)
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        return synthesized.public_send(env, name, argc, args, block);
    }

    return object()->public_send(env, name, argc, args, block);
}

Value Value::send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    PROFILED_SEND(send)
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        return synthesized.send(env, name, argc, args, block);
    }
    if (m_type == Type::Double && FloatObject::optimized_method(name)) {
        if (argc > 0 && args[0].is_fast_float())
            args[0].guard();
        auto synthesized = FloatObject { m_double };
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
        auto i = m_integer;
        m_object = new IntegerObject { i };
        m_type = Type::Pointer;
        if (was_gc_enabled) Heap::the().gc_enable();
        break;
    }
    case Type::Double:
        m_type = Type::Pointer;
        m_object = new FloatObject { m_double };
        break;
    case Type::Pointer:
        break;
    }
}

#undef PROFILED_SEND

}
