#include "natalie.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/object_type.hpp"

namespace Natalie {

Value::Value(nat_int_t integer)
    : Value { Integer(integer) } { }

Value::Value(const Integer integer)
    : m_value { integer.m_value } { }

Value Value::integer(TM::String &&str) {
    return Integer(std::move(str));
}

#define PROFILED_SEND(type)                                                             \
    static auto is_profiled = NativeProfiler::the()->enabled();                         \
    NativeProfilerEvent *event = nullptr;                                               \
    if (is_profiled) {                                                                  \
        auto classnameOf = [](Value val) -> String {                                    \
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
Value Value::immediate_send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from, MethodVisibility visibility) {
    auto method_info = klass()->find_method(env, name);
    if (!method_info.is_defined()) {
        // FIXME: store missing reason on current thread
        GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Undefined);
        if (respond_to(env, "method_missing"_s)) {
            Vector<Value> new_args(args.size() + 1);
            new_args.push(name);
            for (size_t i = 0; i < args.size(); i++)
                new_args.push(args[i]);
            return send(env, "method_missing"_s, Args(new_args, args.has_keyword_hash()), block);
        } else {
            env->raise_no_method_error(*this, name, GlobalEnv::the()->method_missing_reason());
        }
    }

    args.pop_empty_keyword_hash();
    return method_info.method()->call(env, *this, std::move(args), block);
}

ClassObject *Value::klass() const {
    if (is_integer())
        return GlobalEnv::the()->Integer();
    if (is_nil())
        return GlobalEnv::the()->Object()->const_fetch("NilClass"_s).as_class();
    auto ptr = object();
    if (ptr)
        return ptr->klass();
    return nullptr;
}

ClassObject *Value::singleton_class() const {
    if (is_integer() || is_float() || is_symbol())
        return nullptr;
    if (is_nil())
        return GlobalEnv::the()->Object()->const_fetch("NilClass"_s).as_class();
    auto ptr = object();
    if (ptr)
        return ptr->singleton_class();
    return nullptr;
}

ClassObject *Value::singleton_class(Env *env) {
    return Object::singleton_class(env, *this);
}

StringObject *Value::to_str(Env *env) {
    if (is_integer()) {
        if (respond_to(env, "to_str"_s))
            return send(env, "to_str"_s).as_string_or_raise(env);
        else
            env->raise_type_error(*this, "String");
    }

    if (is_string())
        return as_string();

    auto to_str = "to_str"_s;
    if (!respond_to(env, to_str))
        env->raise_type_error(*this, "String");

    auto result = send(env, to_str);

    if (result.is_string())
        return result.as_string();

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
            return send(env, "to_str"_s).as_string_or_raise(env);
        else
            env->raise_type_error(*this, "String");
    }

    if (is_string())
        return as_string();

    auto to_str = "to_str"_s;
    if (!respond_to(env, to_str))
        env->raise_type_error2(*this, "String");

    auto result = send(env, to_str);

    if (result.is_string())
        return result.as_string();

    env->raise(
        "TypeError", "can't convert {} to String ({}#to_str gives {})",
        klass()->inspect_str(),
        klass()->inspect_str(),
        result.klass()->inspect_str());
}

Value Value::public_send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::PUBLIC_SEND);

    if (is_integer() || is_nil())
        return immediate_send(env, name, std::move(args), block, sent_from, MethodVisibility::Public);

    return object()->public_send(env, name, std::move(args), block, sent_from);
}

Value Value::send(Env *env, SymbolObject *name, Args &&args, Block *block, Value sent_from) {
    PROFILED_SEND(NativeProfilerEvent::Type::SEND);

    if (is_integer() || is_nil())
        return immediate_send(env, name, std::move(args), block, sent_from, MethodVisibility::Private);

    return object()->send(env, name, std::move(args), block, sent_from);
}

Integer Value::integer() const {
    if (!is_integer()) {
        fprintf(stderr, "Fatal: cannot convert Value of type Pointer to Integer\n");
        abort();
    }
    if (is_fixnum())
        return static_cast<nat_int_t>(m_value) >> 1;
    return Integer(reinterpret_cast<BigInt *>(m_value));
}

Integer Value::integer_or_raise(Env *env) const {
    if (!is_integer())
        raise_type_error(env, "Integer");
    return integer();
}

ObjectType Value::type() const {
    if (is_integer())
        return ObjectType::Integer;
    if (is_nil())
        return ObjectType::Nil;
    return pointer()->type();
}

bool Value::is_integer() const {
    if (is_fixnum())
        return true;
    if (!is_pointer())
        return false;
    auto ptr = reinterpret_cast<Object *>(m_value);
    return ptr != 0x0 && reinterpret_cast<Object *>(ptr)->type() == ObjectType::BigInt;
}

bool Value::is_frozen() const {
    return is_integer() || is_float() || is_nil() || object()->is_frozen();
}

bool Value::has_instance_variables() const {
    switch (type()) {
    case Object::Type::False:
    case Object::Type::Float:
    case Object::Type::Integer:
    case Object::Type::Nil:
    case Object::Type::Symbol:
    case Object::Type::True:
        return false;
    default:
        return true;
    }
}

void Value::assert_integer(Env *env) const {
    if (is_integer())
        return;
    raise_type_error(env, "Integer");
}

void Value::assert_type(Env *env, ObjectType expected_type, const char *expected_class_name) const {
    assert(expected_type != ObjectType::Integer);
    assert(expected_type != ObjectType::Nil);
    if (type() != expected_type)
        raise_type_error(env, expected_class_name);
}

void Value::assert_not_frozen(Env *env) const {
    if (is_integer())
        env->raise("FrozenError", "can't modify frozen Integer: {}", integer().to_string());
    if (is_nil())
        env->raise("FrozenError", "can't modify frozen NilClass: nil");

    object()->assert_not_frozen(env);
}

[[noreturn]] void Value::raise_type_error(Env *env, const char *expected_class_name) const {
    if (is_integer())
        env->raise("TypeError", "no implicit conversion of Integer into {}", expected_class_name);
    if (is_nil())
        env->raise_type_error(Value::nil(), expected_class_name);
    env->raise_type_error(object(), expected_class_name);
}

bool Value::is_a(Env *env, Value val) const {
    if (!val.is_module())
        return false;

    ModuleObject *module = val.as_module();
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

bool Value::is_true() const { return is_pointer() && pointer()->type() == ObjectType::True; }
bool Value::is_false() const { return is_pointer() && pointer()->type() == ObjectType::False; }
bool Value::is_fiber() const { return is_pointer() && pointer()->type() == ObjectType::Fiber; }
bool Value::is_enumerator_arithmetic_sequence() const { return is_pointer() && pointer()->type() == ObjectType::EnumeratorArithmeticSequence; }
bool Value::is_array() const { return is_pointer() && pointer()->type() == ObjectType::Array; }
bool Value::is_binding() const { return is_pointer() && pointer()->type() == ObjectType::Binding; }
bool Value::is_method() const { return is_pointer() && pointer()->type() == ObjectType::Method; }
bool Value::is_module() const { return is_pointer() && (pointer()->type() == ObjectType::Module || pointer()->type() == ObjectType::Class); }
bool Value::is_class() const { return is_pointer() && pointer()->type() == ObjectType::Class; }
bool Value::is_complex() const { return is_pointer() && pointer()->type() == ObjectType::Complex; }
bool Value::is_dir() const { return is_pointer() && pointer()->type() == ObjectType::Dir; }
bool Value::is_encoding() const { return is_pointer() && pointer()->type() == ObjectType::Encoding; }
bool Value::is_env() const { return is_pointer() && pointer()->type() == ObjectType::Env; }
bool Value::is_exception() const { return is_pointer() && pointer()->type() == ObjectType::Exception; }
bool Value::is_float() const { return is_pointer() && pointer()->type() == ObjectType::Float; }
bool Value::is_hash() const { return is_pointer() && pointer()->type() == ObjectType::Hash; }
bool Value::is_io() const { return is_pointer() && (pointer()->type() == ObjectType::Io || pointer()->type() == ObjectType::File); }
bool Value::is_file() const { return is_pointer() && pointer()->type() == ObjectType::File; }
bool Value::is_file_stat() const { return is_pointer() && pointer()->type() == ObjectType::FileStat; }
bool Value::is_match_data() const { return is_pointer() && pointer()->type() == ObjectType::MatchData; }
bool Value::is_proc() const { return is_pointer() && pointer()->type() == ObjectType::Proc; }
bool Value::is_random() const { return is_pointer() && pointer()->type() == ObjectType::Random; }
bool Value::is_range() const { return is_pointer() && pointer()->type() == ObjectType::Range; }
bool Value::is_rational() const { return is_pointer() && pointer()->type() == ObjectType::Rational; }
bool Value::is_regexp() const { return is_pointer() && pointer()->type() == ObjectType::Regexp; }
bool Value::is_symbol() const { return is_pointer() && pointer()->type() == ObjectType::Symbol; }
bool Value::is_string() const { return is_pointer() && pointer()->type() == ObjectType::String; }
bool Value::is_thread() const { return is_pointer() && pointer()->type() == ObjectType::Thread; }
bool Value::is_thread_backtrace_location() const { return is_pointer() && pointer()->type() == ObjectType::ThreadBacktraceLocation; }
bool Value::is_thread_group() const { return is_pointer() && pointer()->type() == ObjectType::ThreadGroup; }
bool Value::is_thread_mutex() const { return is_pointer() && pointer()->type() == ObjectType::ThreadMutex; }
bool Value::is_time() const { return is_pointer() && pointer()->type() == ObjectType::Time; }
bool Value::is_unbound_method() const { return is_pointer() && pointer()->type() == ObjectType::UnboundMethod; }
bool Value::is_void_p() const { return is_pointer() && pointer()->type() == ObjectType::VoidP; }

bool Value::is_truthy() const { return !is_falsey(); }
bool Value::is_falsey() const { return is_false() || is_nil(); }
bool Value::is_numeric() const { return is_integer() || is_float(); }
bool Value::is_boolean() const { return is_true() || is_false(); }

Enumerator::ArithmeticSequenceObject *Value::as_enumerator_arithmetic_sequence() const {
    assert(is_enumerator_arithmetic_sequence());
    return reinterpret_cast<Enumerator::ArithmeticSequenceObject *>(m_value);
}

ArrayObject *Value::as_array() const {
    assert(is_array());
    return reinterpret_cast<ArrayObject *>(m_value);
}

BindingObject *Value::as_binding() const {
    assert(is_binding());
    return reinterpret_cast<BindingObject *>(m_value);
}

MethodObject *Value::as_method() const {
    assert(is_method());
    return reinterpret_cast<MethodObject *>(m_value);
}

ModuleObject *Value::as_module() const {
    assert(is_module());
    return reinterpret_cast<ModuleObject *>(m_value);
}

ClassObject *Value::as_class() const {
    assert(is_class());
    return reinterpret_cast<ClassObject *>(m_value);
}

ComplexObject *Value::as_complex() const {
    assert(is_complex());
    return reinterpret_cast<ComplexObject *>(m_value);
}

DirObject *Value::as_dir() const {
    assert(is_dir());
    return reinterpret_cast<DirObject *>(m_value);
}

EncodingObject *Value::as_encoding() const {
    assert(is_encoding());
    return reinterpret_cast<EncodingObject *>(m_value);
}

EnvObject *Value::as_env() const {
    assert(is_env());
    return reinterpret_cast<EnvObject *>(m_value);
}

ExceptionObject *Value::as_exception() const {
    assert(is_exception());
    return reinterpret_cast<ExceptionObject *>(m_value);
}

FalseObject *Value::as_false() const {
    assert(is_false());
    return reinterpret_cast<FalseObject *>(m_value);
}

FiberObject *Value::as_fiber() const {
    assert(is_fiber());
    return reinterpret_cast<FiberObject *>(m_value);
}

FloatObject *Value::as_float() const {
    assert(is_float());
    return reinterpret_cast<FloatObject *>(m_value);
}

HashObject *Value::as_hash() const {
    assert(is_hash());
    return reinterpret_cast<HashObject *>(m_value);
}

IoObject *Value::as_io() const {
    assert(is_io());
    return reinterpret_cast<IoObject *>(m_value);
}

FileObject *Value::as_file() const {
    assert(is_file());
    return reinterpret_cast<FileObject *>(m_value);
}

FileStatObject *Value::as_file_stat() const {
    assert(is_file_stat());
    return reinterpret_cast<FileStatObject *>(m_value);
}

MatchDataObject *Value::as_match_data() const {
    assert(is_match_data());
    return reinterpret_cast<MatchDataObject *>(m_value);
}

ProcObject *Value::as_proc() const {
    assert(is_proc());
    return reinterpret_cast<ProcObject *>(m_value);
}

RandomObject *Value::as_random() const {
    assert(is_random());
    return reinterpret_cast<RandomObject *>(m_value);
}

RangeObject *Value::as_range() const {
    assert(is_range());
    return reinterpret_cast<RangeObject *>(m_value);
}

RationalObject *Value::as_rational() const {
    assert(is_rational());
    return reinterpret_cast<RationalObject *>(m_value);
}

RegexpObject *Value::as_regexp() const {
    assert(is_regexp());
    return reinterpret_cast<RegexpObject *>(m_value);
}

StringObject *Value::as_string() const {
    assert(is_string());
    return reinterpret_cast<StringObject *>(m_value);
}

SymbolObject *Value::as_symbol() const {
    assert(is_symbol());
    return reinterpret_cast<SymbolObject *>(m_value);
}

ThreadObject *Value::as_thread() const {
    assert(is_thread());
    return reinterpret_cast<ThreadObject *>(m_value);
}

Thread::Backtrace::LocationObject *Value::as_thread_backtrace_location() const {
    assert(is_thread_backtrace_location());
    return reinterpret_cast<Thread::Backtrace::LocationObject *>(m_value);
}

ThreadGroupObject *Value::as_thread_group() const {
    assert(is_thread_group());
    return reinterpret_cast<ThreadGroupObject *>(m_value);
}

Thread::MutexObject *Value::as_thread_mutex() const {
    assert(is_thread_mutex());
    return reinterpret_cast<Thread::MutexObject *>(m_value);
}

TimeObject *Value::as_time() const {
    assert(is_time());
    return reinterpret_cast<TimeObject *>(m_value);
}

TrueObject *Value::as_true() const {
    assert(is_true());
    return reinterpret_cast<TrueObject *>(m_value);
}

UnboundMethodObject *Value::as_unbound_method() const {
    assert(is_unbound_method());
    return reinterpret_cast<UnboundMethodObject *>(m_value);
}

VoidPObject *Value::as_void_p() const {
    assert(is_void_p());
    return reinterpret_cast<VoidPObject *>(m_value);
}

ArrayObject *Value::as_array_or_raise(Env *env) const {
    if (!is_array())
        env->raise("TypeError", "{} can't be coerced into Array", klass()->inspect_str());
    return reinterpret_cast<ArrayObject *>(m_value);
}

ClassObject *Value::as_class_or_raise(Env *env) const {
    if (!is_class())
        env->raise("TypeError", "{} can't be coerced into Class", klass()->inspect_str());
    return reinterpret_cast<ClassObject *>(m_value);
}

EncodingObject *Value::as_encoding_or_raise(Env *env) const {
    if (!is_encoding())
        env->raise("TypeError", "{} can't be coerced into Encoding", klass()->inspect_str());
    return reinterpret_cast<EncodingObject *>(m_value);
}

ExceptionObject *Value::as_exception_or_raise(Env *env) const {
    if (!is_exception())
        env->raise("TypeError", "{} can't be coerced into Exception", klass()->inspect_str());
    return reinterpret_cast<ExceptionObject *>(m_value);
}

FloatObject *Value::as_float_or_raise(Env *env) const {
    if (!is_float())
        env->raise("TypeError", "{} can't be coerced into Float", klass()->inspect_str());
    return reinterpret_cast<FloatObject *>(m_value);
}

HashObject *Value::as_hash_or_raise(Env *env) const {
    if (!is_hash())
        env->raise("TypeError", "{} can't be coerced into Hash", klass()->inspect_str());
    return reinterpret_cast<HashObject *>(m_value);
}

MatchDataObject *Value::as_match_data_or_raise(Env *env) const {
    if (!is_match_data())
        env->raise("TypeError", "{} can't be coerced into MatchData", klass()->inspect_str());
    return reinterpret_cast<MatchDataObject *>(m_value);
}

ModuleObject *Value::as_module_or_raise(Env *env) const {
    if (!is_module())
        env->raise("TypeError", "{} can't be coerced into Module", klass()->inspect_str());
    return reinterpret_cast<ModuleObject *>(m_value);
}

RangeObject *Value::as_range_or_raise(Env *env) const {
    if (!is_range())
        env->raise("TypeError", "{} can't be coerced into Range", klass()->inspect_str());
    return reinterpret_cast<RangeObject *>(m_value);
}

StringObject *Value::as_string_or_raise(Env *env) const {
    if (!is_string())
        env->raise("TypeError", "{} can't be coerced into String", klass()->inspect_str());
    return reinterpret_cast<StringObject *>(m_value);
}

ArrayObject *Value::to_ary(Env *env) {
    if (is_array())
        return as_array();

    auto original_class = klass()->inspect_str();

    auto to_ary = "to_ary"_s;

    if (!respond_to(env, to_ary)) {
        if (is_nil())
            env->raise("TypeError", "no implicit conversion of nil into Array");
        env->raise("TypeError", "no implicit conversion of {} into Array", original_class);
    }

    Value val = send(env, to_ary);

    if (val.is_array()) {
        return val.as_array();
    }

    env->raise(
        "TypeError", "can't convert {} to Array ({}#to_ary gives {})",
        original_class,
        original_class,
        val.klass()->inspect_str());
}

IoObject *Value::to_io(Env *env) {
    if (is_io())
        return as_io();

    auto to_io = "to_io"_s;
    if (!respond_to(env, to_io))
        assert_type(env, Object::Type::Io, "IO");

    auto result = send(env, to_io);

    if (result.is_io())
        return result.as_io();

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
        assert_integer(env);

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
        return as_float();

    auto to_f = "to_f"_s;
    if (!respond_to(env, to_f))
        assert_type(env, Object::Type::Float, "Float");

    auto result = send(env, to_f);
    result.assert_type(env, Object::Type::Float, "Float");
    return result.as_float();
}

HashObject *Value::to_hash(Env *env) {
    if (is_hash())
        return as_hash();

    auto original_class = klass()->inspect_str();

    auto to_hash = "to_hash"_s;

    if (!respond_to(env, to_hash)) {
        if (is_nil())
            env->raise("TypeError", "no implicit conversion of nil into Hash");
        env->raise("TypeError", "no implicit conversion of {} into Hash", original_class);
    }

    Value val = send(env, to_hash);

    if (val.is_hash()) {
        return val.as_hash();
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

    return str.as_string();
}

SymbolObject *Value::to_symbol(Env *env, Conversion conversion) {
    if (is_integer()) {
        if (conversion == Conversion::NullAllowed)
            return nullptr;
        env->raise("TypeError", "{} is not a symbol nor a string", inspect_str(env));
    }

    if (is_symbol())
        return as_symbol();
    else if (is_string() || respond_to(env, "to_str"_s))
        return to_str(env)->to_symbol(env);
    else if (conversion == Conversion::NullAllowed)
        return nullptr;
    else
        env->raise("TypeError", "{} is not a symbol nor a string", inspect_str(env));
}

String Value::inspect_str(Env *env) {
    if (!respond_to(env, "inspect"_s))
        return String::format("#<{}:{}>", klass()->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed));
    auto inspected = send(env, "inspect"_s);
    if (!inspected.is_string())
        return ""; // TODO: what to do here?
    return inspected.as_string()->string();
}

String Value::dbg_inspect() const {
    if (is_integer())
        return integer().to_string();
    if (is_nil())
        return "nil";
    return object()->dbg_inspect();
}

#undef PROFILED_SEND

}
