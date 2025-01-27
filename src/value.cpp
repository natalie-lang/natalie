#include "natalie.hpp"
#include "natalie/integer_object.hpp"

namespace Natalie {

#define PROFILED_SEND(type)                                                             \
    static auto is_profiled = NativeProfiler::the()->enabled();                         \
    NativeProfilerEvent *event = nullptr;                                               \
    if (is_profiled) {                                                                  \
        auto classnameOf = [](Value val) -> String {                                    \
            if (val.m_type == Type::Integer)                                            \
                return String("FastInteger");                                           \
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

// FIXME: this doesn't check method visibility, but no tests are failing yet :-)
Value Value::integer_send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from, MethodVisibility visibility) {
    auto method_info = GlobalEnv::the()->Integer()->find_method(env, name);
    if (!method_info.is_defined()) {
        // FIXME: store missing reason on current thread
        GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Undefined);
        auto obj = object();
        if (obj->respond_to(env, "method_missing"_s))
            return obj->method_missing_send(env, name, std::move(args), block);
        else
            env->raise_no_method_error(obj, name, GlobalEnv::the()->method_missing_reason());
    }

    return method_info.method()->call(env, *this, std::move(args), block);
}

Value Value::public_send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::PUBLIC_SEND);

    if (m_type == Type::Integer)
        return integer_send(env, name, std::move(args), block, sent_from, MethodVisibility::Public);

    return m_object->public_send(env, name, std::move(args), block, sent_from);
}

Value Value::send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::SEND);

    if (m_type == Type::Integer)
        return integer_send(env, name, std::move(args), block, sent_from, MethodVisibility::Private);

    return m_object->send(env, name, std::move(args), block, sent_from);
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
    case Type::Pointer:
        break;
    }

    if (garbage_collection_enabled)
        Heap::the().gc_enable();
}

nat_int_t Value::as_fast_integer() const {
    assert(m_type == Type::Integer || (m_type == Type::Pointer && m_object->type() == Object::Type::Integer));
    if (m_type == Type::Integer)
        return m_integer.to_nat_int_t();
    return IntegerObject::to_nat_int_t(static_cast<IntegerObject *>(m_object));
}

bool Value::operator==(Value other) const {
    switch (m_type) {
    case Type::Integer: {
        switch (other.m_type) {
        case Type::Integer:
            return m_integer == other.m_integer;
        default: {
            if (other && other->type() == Object::Type::Integer) {
                auto i = other.integer();
                if (i.is_fixnum())
                    return m_integer == i.to_nat_int_t();
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
        assert(m_object->type() == Object::Type::Integer);
        return IntegerObject::integer(static_cast<IntegerObject *>(m_object));
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
        assert(m_object->type() == Object::Type::Integer);
        return IntegerObject::integer(static_cast<IntegerObject *>(m_object));
        break;
    default:
        NAT_UNREACHABLE();
    }
}

const Integer &Value::integer_or_raise(Env *env) const {
    switch (m_type) {
    case Type::Integer:
        return m_integer;
    case Type::Pointer:
        assert(m_object->type() == Object::Type::Integer);
        return IntegerObject::integer(static_cast<IntegerObject *>(m_object));
        break;
    default:
        env->raise("TypeError", "{} can't be coerced into Integer", m_object->klass()->inspect_str());
    }
}

Integer &Value::integer_or_raise(Env *env) {
    switch (m_type) {
    case Type::Integer:
        return m_integer;
    case Type::Pointer:
        if (m_object->type() == Object::Type::Integer)
            return IntegerObject::integer(static_cast<IntegerObject *>(m_object));
        env->raise("TypeError", "{} can't be coerced into Integer", m_object->klass()->inspect_str());
    }
    NAT_UNREACHABLE();
}

__attribute__((no_sanitize("undefined"))) static nat_int_t left_shift_with_undefined_behavior(nat_int_t x, nat_int_t y) {
    return x << y;
}

nat_int_t Value::object_id() const {
    if (is_integer()) {
        auto i = integer();
        if (i.is_fixnum()) {
            /* Recreate the logic from Ruby: Use a long as tagged pointer, where
             * the rightmost bit is 1, and the remaining bits are the number shifted
             * one right.
             * The regular object ids are the actual memory addresses, these are at
             * least 8 bit aligned, so the rightmost bit will never be set. This
             * means we don't risk duplicate object ids for different objects.
             */
            auto val = i.to_nat_int_t();
            if (val >= (LONG_MIN >> 1) && val <= (LONG_MAX >> 1))
                return left_shift_with_undefined_behavior(val, 1) | 1;
        } else {
            return reinterpret_cast<nat_int_t>(i.bigint_pointer());
        }
    }

    assert(m_type == Type::Pointer);
    assert(m_object);

    return reinterpret_cast<nat_int_t>(m_object);
}

void Value::assert_type(Env *env, ObjectType expected_type, const char *expected_class_name) const {
    switch (m_type) {
    case Type::Integer:
        if (expected_type != Object::Type::Integer)
            env->raise("TypeError", "no implicit conversion of Integer into {}", expected_class_name);
        break;
    case Type::Pointer:
        if (m_object->type() != expected_type)
            env->raise_type_error(m_object, expected_class_name);
    }
}

bool Value::is_integer() const {
    switch (m_type) {
    case Type::Integer:
        return true;
    case Type::Pointer:
        return m_object && m_object->type() == Object::Type::Integer;
    default:
        return false;
    }
}

bool Value::is_nil() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Nil; }
bool Value::is_true() const { return m_type == Type::Pointer && m_object->type() == ObjectType::True; }
bool Value::is_false() const { return m_type == Type::Pointer && m_object->type() == ObjectType::False; }
bool Value::is_fiber() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Fiber; }
bool Value::is_enumerator_arithmetic_sequence() const { return m_type == Type::Pointer && m_object->type() == ObjectType::EnumeratorArithmeticSequence; }
bool Value::is_array() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Array; }
bool Value::is_binding() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Binding; }
bool Value::is_method() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Method; }
bool Value::is_module() const { return m_type == Type::Pointer && (m_object->type() == ObjectType::Module || m_object->type() == ObjectType::Class); }
bool Value::is_class() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Class; }
bool Value::is_complex() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Complex; }
bool Value::is_dir() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Dir; }
bool Value::is_encoding() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Encoding; }
bool Value::is_env() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Env; }
bool Value::is_exception() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Exception; }
bool Value::is_float() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Float; }
bool Value::is_hash() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Hash; }
bool Value::is_io() const { return m_type == Type::Pointer && (m_object->type() == ObjectType::Io || m_object->type() == ObjectType::File); }
bool Value::is_file() const { return m_type == Type::Pointer && m_object->type() == ObjectType::File; }
bool Value::is_file_stat() const { return m_type == Type::Pointer && m_object->type() == ObjectType::FileStat; }
bool Value::is_match_data() const { return m_type == Type::Pointer && m_object->type() == ObjectType::MatchData; }
bool Value::is_proc() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Proc; }
bool Value::is_random() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Random; }
bool Value::is_range() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Range; }
bool Value::is_rational() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Rational; }
bool Value::is_regexp() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Regexp; }
bool Value::is_symbol() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Symbol; }
bool Value::is_string() const { return m_type == Type::Pointer && m_object->type() == ObjectType::String; }
bool Value::is_thread() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Thread; }
bool Value::is_thread_backtrace_location() const { return m_type == Type::Pointer && m_object->type() == ObjectType::ThreadBacktraceLocation; }
bool Value::is_thread_group() const { return m_type == Type::Pointer && m_object->type() == ObjectType::ThreadGroup; }
bool Value::is_thread_mutex() const { return m_type == Type::Pointer && m_object->type() == ObjectType::ThreadMutex; }
bool Value::is_time() const { return m_type == Type::Pointer && m_object->type() == ObjectType::Time; }
bool Value::is_unbound_method() const { return m_type == Type::Pointer && m_object->type() == ObjectType::UnboundMethod; }
bool Value::is_void_p() const { return m_type == Type::Pointer && m_object->type() == ObjectType::VoidP; }

bool Value::is_truthy() const { return !is_false() && !is_nil(); }
bool Value::is_falsey() const { return !is_truthy(); }
bool Value::is_numeric() const { return is_integer() || is_float(); }
bool Value::is_boolean() const { return is_true() || is_false(); }

#undef PROFILED_SEND

}
