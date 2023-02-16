#include "natalie.hpp"

namespace Natalie {

Value Value::floatingpoint(double value) {
    if (isnan(value))
        return FloatObject::nan();
    return Value { value };
}

#define PROFILED_SEND(type)                                                             \
    static auto is_profiled = NativeProfiler::the()->enabled();                         \
    NativeProfilerEvent *event;                                                         \
    if (is_profiled) {                                                                  \
        auto classnameOf = [](Value val) -> String {                                    \
            if (val.m_type == Type::Integer)                                            \
                return String("FastInteger");                                           \
            if (val.m_type == Type::Double)                                             \
                return String("FastDouble");                                            \
            auto maybe_classname = val->klass()->class_name();                          \
            if (maybe_classname.present())                                              \
                return maybe_classname.value();                                         \
            else                                                                        \
                return String("Anonymous");                                             \
        };                                                                              \
        auto event_name = String::format("{}.{}(", classnameOf(*this), name->string()); \
        for (size_t i = 0; i < args.size(); ++i) {                                      \
            if (i > 0)                                                                  \
                event_name.append_char(',');                                            \
            event_name.append(classnameOf(args[i]));                                    \
        }                                                                               \
        event_name.append_char(')');                                                    \
        event = NativeProfilerEvent::named(type, event_name)->start_now();              \
    }                                                                                   \
    Defer log_event([&]() {                                                             \
        auto source_filename = env->file();                                             \
        auto source_line = env->line();                                                 \
        if (is_profiled)                                                                \
            NativeProfiler::the()->push(event->end_now());                              \
    });

template <typename Callback>
Value Value::on_object_value(Callback &&callback) {
    if (m_type == Type::Pointer)
        return callback(*object());

    if (m_type == Type::Integer) {
        auto synthesized_receiver = IntegerObject { m_integer };
        synthesized_receiver.add_synthesized_flag();
        return callback(synthesized_receiver);
    }

    assert(m_type == Type::Double);
    auto synthesized_receiver = FloatObject { m_double };
    synthesized_receiver.add_synthesized_flag();
    return callback(synthesized_receiver);
}

Value Value::public_send(Env *env, SymbolObject *name, Args args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::PUBLIC_SEND);

    return on_object_value([&](Object &object) {
        return object.public_send(env, name, args, block, sent_from);
    });
}

Value Value::send(Env *env, SymbolObject *name, Args args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::SEND);

    return on_object_value([&](Object &object) {
        return object.send(env, name, args, block, sent_from);
    });
}

void Value::hydrate() {
    // Running GC while we're in the processes of hydrating this Value makes
    // debugging VERY confusing. Maybe someday we can remove this GC stuff...
    bool garbage_collection_enabled = Heap::the().gc_enabled();
    if (garbage_collection_enabled)
        Heap::the().gc_disable();

    switch (m_type) {
    case Type::Integer: {
        auto i = m_integer;
        m_object = new IntegerObject { i };
        m_type = Type::Pointer;
        break;
    }
    case Type::Double: {
        auto d = m_double;
        m_object = new FloatObject { d };
        m_type = Type::Pointer;
        break;
    }
    case Type::Pointer:
        break;
    }

    if (garbage_collection_enabled)
        Heap::the().gc_enable();
}

double Value::as_double() const {
    assert(m_type == Type::Double || (m_type == Type::Pointer && m_object->is_float()));
    if (m_type == Type::Double)
        return m_double;
    return m_object->as_float()->to_double();
}

nat_int_t Value::as_fast_integer() const {
    assert(m_type == Type::Integer || (m_type == Type::Pointer && m_object->is_integer()));
    if (m_type == Type::Integer)
        return m_integer;
    return m_object->as_integer()->to_nat_int_t();
}

#undef PROFILED_SEND

}
