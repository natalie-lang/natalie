#include "natalie.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/object_type.hpp"

namespace Natalie {

#define PROFILED_SEND(type)                                                             \
    static auto is_profiled = NativeProfiler::the()->enabled();                         \
    NativeProfilerEvent *event = nullptr;                                               \
    if (is_profiled) {                                                                  \
        auto classnameOf = [](Value val) -> String {                                    \
            if (val.m_type == Type::Integer)                                            \
                return String("FastInteger");                                           \
            auto maybe_classname = val.klass()->name();                                 \
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

    args.pop_empty_keyword_hash();
    return method_info.method()->call(env, *this, std::move(args), block);
}

ClassObject *Value::klass() const {
    switch (m_type) {
    case Type::Integer:
        return GlobalEnv::the()->Integer();
    case Type::Pointer:
        if (m_object)
            return m_object->klass();
    }
    return nullptr;
}

ClassObject *Value::singleton_class() const {
    switch (m_type) {
    case Type::Integer:
        return nullptr;
    case Type::Pointer:
        if (m_object)
            return m_object->singleton_class();
    }
    return nullptr;
}

ClassObject *Value::singleton_class(Env *env) {
    return Object::singleton_class(env, *this);
}

StringObject *Value::to_str(Env *env) {
    if (is_integer()) {
        if (respond_to(env, "to_str"_s))
            return send(env, "to_str"_s)->as_string_or_raise(env);
        else
            env->raise_type_error(*this, "String");
    }

    if (is_string()) return m_object->as_string();

    auto to_str = "to_str"_s;
    if (!respond_to(env, to_str))
        env->raise_type_error(*this, "String");

    auto result = send(env, to_str);

    if (result.is_string())
        return result->as_string();

    env->raise(
        "TypeError", "can't convert {} to String ({}#to_str gives {})",
        klass()->inspect_str(),
        klass()->inspect_str(),
        result.klass()->inspect_str());
}

// This is just like Value::to_str, but it raises more consistent error messages.
// We still need the old error messages because CRuby is inconsistent. :-(
StringObject *Value::to_str2(Env *env) {
    if (is_integer()) {
        if (respond_to(env, "to_str"_s))
            return send(env, "to_str"_s)->as_string_or_raise(env);
        else
            env->raise_type_error(*this, "String");
    }

    if (is_string()) return m_object->as_string();

    auto to_str = "to_str"_s;
    if (!respond_to(env, to_str))
        env->raise_type_error2(*this, "String");

    auto result = send(env, to_str);

    if (result.is_string())
        return result->as_string();

    env->raise(
        "TypeError", "can't convert {} to String ({}#to_str gives {})",
        klass()->inspect_str(),
        klass()->inspect_str(),
        result.klass()->inspect_str());
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
            // FIXME: This is incorrect for 64-bit integers and will produce duplicate ids.
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

void Value::assert_not_frozen(Env *env) const {
    if (is_integer())
        env->raise("FrozenError", "can't modify frozen Integer: {}", integer().to_string());

    m_object->assert_not_frozen(env);
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

bool Value::is_a(Env *env, Value val) const {
    if (!val.is_module())
        return false;

    ModuleObject *module = val->as_module();
    if (klass() == module || singleton_class() == module) {
        return true;
    } else {
        ClassObject *the_klass = singleton_class();
        if (!the_klass)
            the_klass = klass();
        return the_klass->ancestors_includes(env, module);
    }
}

bool Value::respond_to(Env *env, SymbolObject *name_val, bool include_all) {
    if (KernelModule::respond_to_method(env, *this, "respond_to?"_s, true))
        return send(env, "respond_to?"_s, { name_val, bool_object(include_all) }).is_truthy();

    // Needed for BasicObject as it does not have an actual respond_to? method
    return KernelModule::respond_to_method(env, *this, name_val, include_all);
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

ArrayObject *Value::to_ary(Env *env) {
    if (is_array())
        return m_object->as_array();

    auto original_class = klass()->inspect_str();

    auto to_ary = "to_ary"_s;

    if (!respond_to(env, to_ary)) {
        if (is_nil())
            env->raise("TypeError", "no implicit conversion of nil into Array");
        env->raise("TypeError", "no implicit conversion of {} into Array", original_class);
    }

    Value val = send(env, to_ary);

    if (val.is_array()) {
        return val->as_array();
    }

    env->raise(
        "TypeError", "can't convert {} to Array ({}#to_ary gives {})",
        original_class,
        original_class,
        val.klass()->inspect_str());
}

IoObject *Value::to_io(Env *env) {
    if (is_io())
        return m_object->as_io();

    auto to_io = "to_io"_s;
    if (!respond_to(env, to_io))
        assert_type(env, Object::Type::Io, "IO");

    auto result = send(env, to_io);

    if (result.is_io())
        return result->as_io();

    env->raise(
        "TypeError", "can't convert {} to IO ({}#to_io gives {})",
        klass()->inspect_str(),
        klass()->inspect_str(),
        result.klass()->inspect_str());
}

Integer Value::to_int(Env *env) {
    if (is_integer())
        return integer();

    auto to_int = "to_int"_s;
    if (!respond_to(env, to_int))
        assert_type(env, Object::Type::Integer, "Integer");

    auto result = send(env, to_int);

    if (result.is_integer())
        return result.integer();

    auto the_klass = klass();
    env->raise(
        "TypeError", "can't convert {} to Integer ({}#to_int gives {})",
        the_klass->inspect_str(),
        the_klass->inspect_str(),
        result.klass()->inspect_str());
}

FloatObject *Value::to_f(Env *env) {
    if (is_float())
        return m_object->as_float();

    auto to_f = "to_f"_s;
    if (!respond_to(env, to_f))
        assert_type(env, Object::Type::Float, "Float");

    auto result = send(env, to_f);
    result.assert_type(env, Object::Type::Float, "Float");
    return result->as_float();
}

HashObject *Value::to_hash(Env *env) {
    if (is_hash())
        return m_object->as_hash();

    auto original_class = klass()->inspect_str();

    auto to_hash = "to_hash"_s;

    if (!respond_to(env, to_hash)) {
        if (is_nil())
            env->raise("TypeError", "no implicit conversion of nil into Hash");
        env->raise("TypeError", "no implicit conversion of {} into Hash", original_class);
    }

    Value val = send(env, to_hash);

    if (val.is_hash()) {
        return val->as_hash();
    }

    env->raise(
        "TypeError", "can't convert {} to Hash ({}#to_hash gives {})",
        original_class,
        original_class,
        val.klass()->inspect_str());
}

StringObject *Value::to_s(Env *env) {
    auto str = send(env, "to_s"_s);
    if (!str.is_string())
        env->raise("TypeError", "no implicit conversion of {} into String", klass()->name());

    return str->as_string();
}

void Value::auto_hydrate() {
    switch (m_type) {
    case Type::Integer: {
#ifdef NAT_NO_HYDRATE
        printf("Fatal: integer hydration is disabled.\n");
        abort();
#else
        hydrate();
#endif
    }
    case Type::Pointer:
        break;
    }
}

String Value::inspect_str(Env *env) {
    if (!respond_to(env, "inspect"_s))
        return String::format("#<{}:{}>", klass()->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed));
    auto inspected = send(env, "inspect"_s);
    if (!inspected.is_string())
        return ""; // TODO: what to do here?
    return inspected->as_string()->string();
}

String Value::dbg_inspect() const {
    switch (m_type) {
    case Type::Integer:
        return m_integer.to_string();
    case Type::Pointer:
        return m_object->dbg_inspect();
    }
    NAT_UNREACHABLE();
}

#undef PROFILED_SEND

}
