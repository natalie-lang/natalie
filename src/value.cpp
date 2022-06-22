#include "natalie.hpp"

namespace Natalie {

Value Value::floatingpoint(double value) {
    if (isnan(value))
        return FloatObject::nan();
    return Value { value };
}

#define PROFILED_SEND(type)                                                                     \
    static auto is_profiled = NativeProfiler::the()->enabled();                                 \
    NativeProfilerEvent *event;                                                                 \
    if (is_profiled) {                                                                          \
        auto classnameOf = [](Value val) {                                                      \
            if (val.m_type == Type::Integer)                                                    \
                return "FastInteger";                                                           \
            if (val.m_type == Type::Double)                                                     \
                return "FastDouble";                                                            \
            auto maybe_classname = val->klass()->class_name();                                  \
            return (maybe_classname.present()) ? maybe_classname.value().c_str() : "Anonymous"; \
        };                                                                                      \
        auto event_name = new ManagedString();                                                  \
        event_name->append_sprintf("%s.%s(", classnameOf(*this), name->c_str());                \
        for (size_t i = 0; i < args.argc; ++i) {                                                \
            if (i > 0)                                                                          \
                event_name->append_char(',');                                                   \
            event_name->append(classnameOf(args[i]));                                           \
        }                                                                                       \
        event_name->append_char(')');                                                           \
        event = NativeProfilerEvent::named(type, event_name)                                    \
                    ->start_now();                                                              \
    }                                                                                           \
    Defer log_event([&]() {                                                                     \
        auto source_filename = env->file();                                                     \
        auto source_line = env->line();                                                         \
        if (is_profiled)                                                                        \
            NativeProfiler::the()->push(event->end_now());                                      \
    });

Value Value::public_send(Env *env, SymbolObject *name, Args args, Block *block) {
    PROFILED_SEND(NativeProfilerEvent::Type::PUBLIC_SEND)
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (args.argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        auto wrapper = Value { &synthesized };
        wrapper.guard();
        return wrapper.public_send(env, name, args, block);
    }
    if (m_type == Type::Double && FloatObject::optimized_method(name)) {
        if (args.argc > 0 && args[0].is_fast_float())
            args[0].guard();
        auto synthesized = FloatObject { m_double };
        auto wrapper = Value { &synthesized };
        wrapper.guard();
        return wrapper.public_send(env, name, args, block);
    }

    return object()->public_send(env, name, args, block);
}

Value Value::send(Env *env, SymbolObject *name, Args args, Block *block) {
    PROFILED_SEND(NativeProfilerEvent::Type::SEND)
    if (m_type == Type::Integer && IntegerObject::optimized_method(name)) {
        if (args.argc > 0 && args[0].is_fast_integer())
            args[0].guard();
        auto synthesized = IntegerObject { m_integer };
        auto wrapper = Value { &synthesized };
        wrapper.guard();
        return wrapper.send(env, name, args, block);
    }
    if (m_type == Type::Double && FloatObject::optimized_method(name)) {
        if (args.argc > 0 && args[0].is_fast_float())
            args[0].guard();
        auto synthesized = FloatObject { m_double };
        auto wrapper = Value { &synthesized };
        wrapper.guard();
        return wrapper.send(env, name, args, block);
    }

    return object()->send(env, name, args, block);
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
    case Type::Double: {
        bool was_gc_enabled = Heap::the().gc_enabled();
        Heap::the().gc_disable();
        auto d = m_double;
        m_object = new FloatObject { d };
        m_type = Type::Pointer;
        if (was_gc_enabled) Heap::the().gc_enable();
        break;
    }
    case Type::Pointer:
        break;
    }
}

#undef PROFILED_SEND

}
