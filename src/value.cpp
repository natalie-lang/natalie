#include "natalie.hpp"
#include "natalie/integer_object.hpp"

namespace Natalie {

Value Value::floatingpoint(double value) {
    if (isnan(value))
        return FloatObject::nan();
    return Value { value };
}

#define PROFILED_SEND(type)                                                             \
    static auto is_profiled = NativeProfiler::the()->enabled();                         \
    NativeProfilerEvent *event = nullptr;                                               \
    if (is_profiled) {                                                                  \
        auto classnameOf = [](Value val) -> String {                                    \
            if (val.m_type == Type::Integer)                                            \
                return String("FastInteger");                                           \
            if (val.m_type == Type::Double)                                             \
                return String("FastDouble");                                            \
            auto maybe_classname = val->klass()->name();                                \
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

Value Value::public_send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::PUBLIC_SEND);

    return on_object_value([&](Object &object) {
        return object.public_send(env, name, std::move(args), block, sent_from);
    });
}

Value Value::send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::SEND);

    return on_object_value([&](Object &object) {
        return object.send(env, name, std::move(args), block, sent_from);
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
        return m_integer.to_nat_int_t();
    return IntegerObject::to_nat_int_t(m_object->as_integer());
}

bool Value::operator==(Value other) const {
    switch (m_type) {
    case Type::Integer: {
        switch (other.m_type) {
        case Type::Integer:
            return m_integer == other.m_integer;
        case Type::Double:
            return false;
        default: {
            if (other && other->is_integer()) {
                auto i = other->as_integer();
                if (IntegerObject::is_fixnum(i))
                    return m_integer == IntegerObject::to_nat_int_t(i);
            }
            return false;
        }
        }
    }
    case Type::Double: {
        switch (other.m_type) {
        case Type::Integer:
            return false;
        case Type::Double:
            return m_double == other.m_double;
        default: {
            if (other && other->is_float()) {
                auto f = other->as_float();
                return m_double == f->to_double();
            }
            return false;
        }
        }
    }
    default:
        return m_object == other.m_object;
    }
}

const Integer &Value::integer() const {
    switch (m_type) {
    case Type::Integer:
        return m_integer;
    case Type::Pointer:
        assert(m_object->is_integer());
        return IntegerObject::integer(m_object->as_integer());
        break;
    default:
        NAT_UNREACHABLE();
    }
}

Integer &Value::integer() {
    switch (m_type) {
    case Type::Integer:
        return m_integer;
    case Type::Pointer:
        assert(m_object->is_integer());
        return IntegerObject::integer(m_object->as_integer());
        break;
    default:
        NAT_UNREACHABLE();
    }
}

#undef PROFILED_SEND

}
