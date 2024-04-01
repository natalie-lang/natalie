#include "natalie.hpp"
#include "natalie/forward.hpp"
#include <cctype>
#include <climits>

namespace Natalie {

Object::Object(const Object &other)
    : m_klass { other.m_klass }
    , m_type { other.m_type }
    , m_singleton_class { nullptr }
    , m_owner { other.m_owner } {
    if (other.m_ivars)
        m_ivars = new TM::Hashmap<SymbolObject *, Value> { *other.m_ivars };
}

Value Object::create(Env *env, ClassObject *klass) {
    if (klass->is_singleton())
        env->raise("TypeError", "can't create instance of singleton class");

    Value obj;

    switch (klass->object_type()) {
    case Object::Type::EnumeratorArithmeticSequence:
        obj = new Enumerator::ArithmeticSequenceObject { klass };
        break;

    case Object::Type::Array:
        obj = new ArrayObject {};
        obj->m_klass = klass;
        break;

    case Object::Type::Class:
        obj = new ClassObject { klass };
        break;

    case Object::Type::Complex:
        obj = new ComplexObject { klass };
        break;

    case Object::Type::Dir:
        obj = new DirObject { klass };
        break;

    case Object::Type::Enumerator:
        obj = new Object { klass };
        break;

    case Object::Type::Exception:
        obj = new ExceptionObject { klass };
        break;

    case Object::Type::Fiber:
        obj = new FiberObject { klass };
        break;

    case Object::Type::Hash:
        obj = new HashObject { klass };
        break;

    case Object::Type::Io:
        obj = new IoObject { klass };
        break;

    case Object::Type::MatchData:
        obj = new MatchDataObject { klass };
        break;

    case Object::Type::Module:
        obj = new ModuleObject { klass };
        break;

    case Object::Type::Object:
        obj = new Object { klass };
        break;

    case Object::Type::Proc:
        obj = new ProcObject { klass };
        break;

    case Object::Type::Random:
        obj = new RandomObject { klass };
        break;

    case Object::Type::Range:
        obj = new RangeObject { klass };
        break;

    case Object::Type::Regexp:
        obj = new RegexpObject { klass };
        break;

    case Object::Type::String:
        obj = new StringObject { klass };
        break;

    case Object::Type::Thread:
        obj = new ThreadObject { klass };
        break;

    case Object::Type::ThreadBacktraceLocation:
        obj = new Thread::Backtrace::LocationObject { klass };
        break;

    case Object::Type::ThreadGroup:
        obj = new ThreadGroupObject { klass };
        break;

    case Object::Type::ThreadMutex:
        obj = new Thread::MutexObject { klass };
        break;

    case Object::Type::Time:
        obj = new TimeObject { klass };
        break;

    case Object::Type::VoidP:
        obj = new VoidPObject { klass };
        break;

    case Object::Type::FileStat:
        obj = new FileStatObject { klass };
        break;

    case Object::Type::Binding:
    case Object::Type::Encoding:
    case Object::Type::False:
    case Object::Type::Float:
    case Object::Type::Integer:
    case Object::Type::Method:
    case Object::Type::Nil:
    case Object::Type::Rational:
    case Object::Type::Symbol:
    case Object::Type::True:
    case Object::Type::UnboundMethod:
        obj = nullptr;
        break;
    }

    return obj;
}

Value Object::_new(Env *env, Value klass_value, Args args, Block *block) {
    Value obj = create(env, klass_value->as_class());
    if (!obj)
        NAT_UNREACHABLE();

    obj->send(env, "initialize"_s, args, block);
    return obj;
}

Value Object::allocate(Env *env, Value klass_value, Args args, Block *block) {
    args.ensure_argc_is(env, 0);

    ClassObject *klass = klass_value->as_class();
    if (!klass->respond_to(env, "allocate"_s))
        env->raise("TypeError", "calling {}.allocate is prohibited", klass->inspect_str());

    Value obj;
    switch (klass->object_type()) {
    case Object::Type::Proc:
    case Object::Type::EnumeratorArithmeticSequence:
        obj = nullptr;
        break;

    default:
        obj = create(env, klass);
        break;
    }

    if (!obj)
        env->raise("TypeError", "allocator undefined for {}", klass->inspect_str());

    return obj;
}

Value Object::initialize(Env *env) {
    return NilObject::the();
}

NilObject *Object::as_nil() {
    assert(is_nil());
    return static_cast<NilObject *>(this);
}

const NilObject *Object::as_nil() const {
    assert(is_nil());
    return static_cast<const NilObject *>(this);
}

Enumerator::ArithmeticSequenceObject *Object::as_enumerator_arithmetic_sequence() {
    assert(is_enumerator_arithmetic_sequence());
    return static_cast<Enumerator::ArithmeticSequenceObject *>(this);
}

ArrayObject *Object::as_array() {
    assert(is_array());
    return static_cast<ArrayObject *>(this);
}

const ArrayObject *Object::as_array() const {
    assert(is_array());
    return static_cast<const ArrayObject *>(this);
}

BindingObject *Object::as_binding() {
    assert(is_binding());
    return static_cast<BindingObject *>(this);
}

const BindingObject *Object::as_binding() const {
    assert(is_binding());
    return static_cast<const BindingObject *>(this);
}

MethodObject *Object::as_method() {
    assert(is_method());
    return static_cast<MethodObject *>(this);
}

const MethodObject *Object::as_method() const {
    assert(is_method());
    return static_cast<const MethodObject *>(this);
}

ModuleObject *Object::as_module() {
    assert(is_module());
    return static_cast<ModuleObject *>(this);
}

const ModuleObject *Object::as_module() const {
    assert(is_module());
    return static_cast<const ModuleObject *>(this);
}

ClassObject *Object::as_class() {
    assert(is_class());
    return static_cast<ClassObject *>(this);
}

const ClassObject *Object::as_class() const {
    assert(is_class());
    return static_cast<const ClassObject *>(this);
}

ComplexObject *Object::as_complex() {
    assert(is_complex());
    return static_cast<ComplexObject *>(this);
}

const ComplexObject *Object::as_complex() const {
    assert(is_complex());
    return static_cast<const ComplexObject *>(this);
}

DirObject *Object::as_dir() {
    assert(is_dir());
    return static_cast<DirObject *>(this);
}

const DirObject *Object::as_dir() const {
    assert(is_dir());
    return static_cast<const DirObject *>(this);
}

EncodingObject *Object::as_encoding() {
    assert(is_encoding());
    return static_cast<EncodingObject *>(this);
}

const EncodingObject *Object::as_encoding() const {
    assert(is_encoding());
    return static_cast<const EncodingObject *>(this);
}

ExceptionObject *Object::as_exception() {
    assert(is_exception());
    return static_cast<ExceptionObject *>(this);
}

const ExceptionObject *Object::as_exception() const {
    assert(is_exception());
    return static_cast<const ExceptionObject *>(this);
}

FalseObject *Object::as_false() {
    assert(is_false());
    return static_cast<FalseObject *>(this);
}

const FalseObject *Object::as_false() const {
    assert(is_false());
    return static_cast<const FalseObject *>(this);
}

FiberObject *Object::as_fiber() {
    assert(is_fiber());
    return static_cast<FiberObject *>(this);
}

const FiberObject *Object::as_fiber() const {
    assert(is_fiber());
    return static_cast<const FiberObject *>(this);
}

FloatObject *Object::as_float() {
    assert(is_float());
    return static_cast<FloatObject *>(this);
}

const FloatObject *Object::as_float() const {
    assert(is_float());
    return static_cast<const FloatObject *>(this);
}

HashObject *Object::as_hash() {
    assert(is_hash());
    return static_cast<HashObject *>(this);
}

const HashObject *Object::as_hash() const {
    assert(is_hash());
    return static_cast<const HashObject *>(this);
}

IntegerObject *Object::as_integer() {
    assert(is_integer());
    return static_cast<IntegerObject *>(this);
}

const IntegerObject *Object::as_integer() const {
    assert(is_integer());
    return static_cast<const IntegerObject *>(this);
}

IoObject *Object::as_io() {
    assert(is_io());
    return static_cast<IoObject *>(this);
}

const IoObject *Object::as_io() const {
    assert(is_io());
    return static_cast<const IoObject *>(this);
}

FileObject *Object::as_file() {
    assert(is_io());
    return static_cast<FileObject *>(this);
}

const FileObject *Object::as_file() const {
    assert(is_io());
    return static_cast<const FileObject *>(this);
}

FileStatObject *Object::as_file_stat() {
    assert(is_file_stat());
    return static_cast<FileStatObject *>(this);
}

const FileStatObject *Object::as_file_stat() const {
    assert(is_file_stat());
    return static_cast<const FileStatObject *>(this);
}

MatchDataObject *Object::as_match_data() {
    assert(is_match_data());
    return static_cast<MatchDataObject *>(this);
}

const MatchDataObject *Object::as_match_data() const {
    assert(is_match_data());
    return static_cast<const MatchDataObject *>(this);
}

ProcObject *Object::as_proc() {
    assert(is_proc());
    return static_cast<ProcObject *>(this);
}

const ProcObject *Object::as_proc() const {
    assert(is_proc());
    return static_cast<const ProcObject *>(this);
}

RandomObject *Object::as_random() {
    assert(is_random());
    return static_cast<RandomObject *>(this);
}

const RandomObject *Object::as_random() const {
    assert(is_random());
    return static_cast<const RandomObject *>(this);
}

RangeObject *Object::as_range() {
    assert(is_range());
    return static_cast<RangeObject *>(this);
}

const RangeObject *Object::as_range() const {
    assert(is_range());
    return static_cast<const RangeObject *>(this);
}

RationalObject *Object::as_rational() {
    assert(is_rational());
    return static_cast<RationalObject *>(this);
}

const RationalObject *Object::as_rational() const {
    assert(is_rational());
    return static_cast<const RationalObject *>(this);
}

RegexpObject *Object::as_regexp() {
    assert(is_regexp());
    return static_cast<RegexpObject *>(this);
}

const RegexpObject *Object::as_regexp() const {
    assert(is_regexp());
    return static_cast<const RegexpObject *>(this);
}

StringObject *Object::as_string() {
    assert(is_string());
    return static_cast<StringObject *>(this);
}

const StringObject *Object::as_string() const {
    assert(is_string());
    return static_cast<const StringObject *>(this);
}

SymbolObject *Object::as_symbol() {
    assert(is_symbol());
    return static_cast<SymbolObject *>(this);
}

const SymbolObject *Object::as_symbol() const {
    assert(is_symbol());
    return static_cast<const SymbolObject *>(this);
}

ThreadObject *Object::as_thread() {
    assert(is_thread());
    return static_cast<ThreadObject *>(this);
}

const ThreadObject *Object::as_thread() const {
    assert(is_thread());
    return static_cast<const ThreadObject *>(this);
}

Thread::Backtrace::LocationObject *Object::as_thread_backtrace_location() {
    assert(is_thread_backtrace_location());
    return static_cast<Thread::Backtrace::LocationObject *>(this);
}

const Thread::Backtrace::LocationObject *Object::as_thread_backtrace_location() const {
    assert(is_thread_backtrace_location());
    return static_cast<const Thread::Backtrace::LocationObject *>(this);
}

ThreadGroupObject *Object::as_thread_group() {
    assert(is_thread_group());
    return static_cast<ThreadGroupObject *>(this);
}

const ThreadGroupObject *Object::as_thread_group() const {
    assert(is_thread_group());
    return static_cast<const ThreadGroupObject *>(this);
}

Thread::MutexObject *Object::as_thread_mutex() {
    assert(is_thread_mutex());
    return static_cast<Thread::MutexObject *>(this);
}

const Thread::MutexObject *Object::as_thread_mutex() const {
    assert(is_thread_mutex());
    return static_cast<const Thread::MutexObject *>(this);
}

TimeObject *Object::as_time() {
    assert(is_time());
    return static_cast<TimeObject *>(this);
}

const TimeObject *Object::as_time() const {
    assert(is_time());
    return static_cast<const TimeObject *>(this);
}

TrueObject *Object::as_true() {
    assert(is_true());
    return static_cast<TrueObject *>(this);
}

const TrueObject *Object::as_true() const {
    assert(is_true());
    return static_cast<const TrueObject *>(this);
}

UnboundMethodObject *Object::as_unbound_method() {
    assert(is_unbound_method());
    return static_cast<UnboundMethodObject *>(this);
}

const UnboundMethodObject *Object::as_unbound_method() const {
    assert(is_unbound_method());
    return static_cast<const UnboundMethodObject *>(this);
}

VoidPObject *Object::as_void_p() {
    assert(is_void_p());
    return static_cast<VoidPObject *>(this);
}

const VoidPObject *Object::as_void_p() const {
    assert(is_void_p());
    return static_cast<const VoidPObject *>(this);
}

KernelModule *Object::as_kernel_module_for_method_binding() {
    return static_cast<KernelModule *>(this);
}

EnvObject *Object::as_env_object_for_method_binding() {
    return static_cast<EnvObject *>(this);
}

ParserObject *Object::as_parser_object_for_method_binding() {
    return static_cast<ParserObject *>(this);
}

ArrayObject *Object::as_array_or_raise(Env *env) {
    if (!is_array())
        env->raise("TypeError", "{} can't be coerced into Array", m_klass->inspect_str());
    return static_cast<ArrayObject *>(this);
}

ClassObject *Object::as_class_or_raise(Env *env) {
    if (!is_class())
        env->raise("TypeError", "{} can't be coerced into Class", m_klass->inspect_str());
    return static_cast<ClassObject *>(this);
}

ExceptionObject *Object::as_exception_or_raise(Env *env) {
    if (!is_exception())
        env->raise("TypeError", "{} can't be coerced into Exception", m_klass->inspect_str());
    return static_cast<ExceptionObject *>(this);
}

FloatObject *Object::as_float_or_raise(Env *env) {
    if (!is_float())
        env->raise("TypeError", "{} can't be coerced into Float", m_klass->inspect_str());
    return static_cast<FloatObject *>(this);
}

HashObject *Object::as_hash_or_raise(Env *env) {
    if (!is_hash())
        env->raise("TypeError", "{} can't be coerced into Hash", m_klass->inspect_str());
    return static_cast<HashObject *>(this);
}

IntegerObject *Object::as_integer_or_raise(Env *env) {
    if (!is_integer())
        env->raise("TypeError", "{} can't be coerced into Integer", m_klass->inspect_str());
    return static_cast<IntegerObject *>(this);
}

MatchDataObject *Object::as_match_data_or_raise(Env *env) {
    if (!is_match_data())
        env->raise("TypeError", "{} can't be coerced into MatchData", m_klass->inspect_str());
    return static_cast<MatchDataObject *>(this);
}

StringObject *Object::as_string_or_raise(Env *env) {
    if (!is_string())
        env->raise("TypeError", "{} can't be coerced into String", m_klass->inspect_str());
    return static_cast<StringObject *>(this);
}

SymbolObject *Object::to_symbol(Env *env, Conversion conversion) {
    if (is_symbol()) {
        return as_symbol();
    } else if (is_string() || respond_to(env, "to_str"_s)) {
        return to_str(env)->to_symbol(env);
    } else if (conversion == Conversion::NullAllowed) {
        return nullptr;
    } else {
        env->raise("TypeError", "{} is not a symbol nor a string", inspect_str(env));
    }
}

SymbolObject *Object::to_instance_variable_name(Env *env) {
    SymbolObject *symbol = to_symbol(env, Conversion::Strict); // TypeError if not Symbol/String

    if (!symbol->is_ivar_name()) {
        if (is_string()) {
            env->raise_name_error(as_string(), "`{}' is not allowed as an instance variable name", symbol->string());
        } else {
            env->raise_name_error(symbol, "`{}' is not allowed as an instance variable name", symbol->string());
        }
    }

    return symbol;
}

void Object::set_singleton_class(ClassObject *klass) {
    klass->set_is_singleton(true);
    m_singleton_class = klass;
}

ClassObject *Object::singleton_class(Env *env) {
    if (m_singleton_class) {
        return m_singleton_class;
    }

    if (is_integer() || is_float() || is_symbol()) {
        env->raise("TypeError", "can't define singleton");
    }

    String name;
    if (is_module()) {
        name = String::format("#<Class:{}>", as_module()->inspect_str());
    } else if (respond_to(env, "inspect"_s)) {
        name = String::format("#<Class:{}>", inspect_str(env));
    }

    ClassObject *singleton_superclass;
    if (is_class()) {
        singleton_superclass = as_class()->superclass(env)->singleton_class(env);
    } else {
        singleton_superclass = m_klass;
    }
    auto new_singleton_class = new ClassObject { singleton_superclass };
    if (is_frozen()) new_singleton_class->freeze();
    singleton_superclass->initialize_subclass_without_checks(new_singleton_class, env, name);
    set_singleton_class(new_singleton_class);
    if (is_frozen()) m_singleton_class->freeze();
    return m_singleton_class;
}

ClassObject *Object::subclass(Env *env, const char *name) {
    if (!is_class())
        env->raise("TypeError", "superclass must be an instance of Class (given an instance of {})", klass()->inspect_str());
    return as_class()->subclass(env, name);
}

Value Object::extend(Env *env, Args args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i]->type() == Object::Type::Module) {
            extend_once(env, args[i]->as_module());
        } else {
            env->raise("TypeError", "wrong argument type {} (expected Module)", args[i]->klass()->inspect_str());
        }
    }
    return this;
}

void Object::extend_once(Env *env, ModuleObject *module) {
    singleton_class(env)->include_once(env, module);
}

Value Object::const_find(Env *env, SymbolObject *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    return m_klass->const_find(env, name, search_mode, failure_mode);
}

Value Object::const_find_with_autoload(Env *env, Value self, SymbolObject *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    return m_klass->const_find_with_autoload(env, self, name, search_mode, failure_mode);
}

Value Object::const_get(SymbolObject *name) const {
    return m_klass->const_get(name);
}

Value Object::const_fetch(SymbolObject *name) {
    return m_klass->const_fetch(name);
}

Value Object::const_set(SymbolObject *name, Value val) {
    return m_klass->const_set(name, val);
}

Value Object::const_set(SymbolObject *name, MethodFnPtr autoload_fn, StringObject *autoload_path) {
    return m_klass->const_set(name, autoload_fn, autoload_path);
}

bool Object::ivar_defined(Env *env, SymbolObject *name) {
    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->string());

    if (!m_ivars)
        return false;

    auto val = m_ivars->get(name, env);
    if (val)
        return true;
    else
        return false;
}

Value Object::ivar_get(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->string());

    if (!m_ivars)
        return NilObject::the();

    auto val = m_ivars->get(name, env);
    if (val)
        return val;
    else
        return NilObject::the();
}

Value Object::ivar_remove(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    if (!name->is_ivar_name())
        env->raise("NameError", "`{}' is not allowed as an instance variable name", name->string());

    if (!m_ivars)
        env->raise("NameError", "instance variable {} not defined", name->string());

    auto val = m_ivars->remove(name, env);
    if (val)
        return val;
    else
        env->raise("NameError", "instance variable {} not defined", name->string());
}

Value Object::ivar_set(Env *env, SymbolObject *name, Value val) {
    NAT_GC_GUARD_VALUE(val);
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    assert_not_frozen(env);

    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->string());

    if (!m_ivars)
        m_ivars = new TM::Hashmap<SymbolObject *, Value> {};

    m_ivars->put(name, val.object(), env);
    return val;
}

Value Object::instance_variables(Env *env) {
    if (m_type == Object::Type::Integer || m_type == Object::Type::Float || !m_ivars) {
        return new ArrayObject;
    }

    ArrayObject *ary = new ArrayObject { m_ivars->size() };

    for (auto pair : *m_ivars)
        ary->push(pair.first);
    return ary;
}

Value Object::cvar_get(Env *env, SymbolObject *name) {
    Value val = cvar_get_or_null(env, name);
    if (val) {
        return val;
    } else {
        ModuleObject *module;
        if (is_module()) {
            module = as_module();
        } else {
            module = m_klass;
        }
        env->raise_name_error(name, "uninitialized class variable {} in {}", name->string(), module->inspect_str());
    }
}

Value Object::cvar_get_or_null(Env *env, SymbolObject *name) {
    return m_klass->cvar_get_or_null(env, name);
}

Value Object::cvar_set(Env *env, SymbolObject *name, Value val) {
    return m_klass->cvar_set(env, name, val);
}

void Object::method_alias(Env *env, Value new_name, Value old_name) {
    new_name->assert_type(env, Type::Symbol, "Symbol");
    old_name->assert_type(env, Type::Symbol, "Symbol");
    method_alias(env, new_name->as_symbol(), old_name->as_symbol());
}

void Object::method_alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    if (is_integer() || is_symbol()) {
        env->raise("TypeError", "no klass to make alias");
    }
    if (is_main_object()) {
        m_klass->make_method_alias(env, new_name, old_name);
    } else if (is_module()) {
        as_module()->method_alias(env, new_name, old_name);
    } else {
        singleton_class(env)->make_method_alias(env, new_name, old_name);
    }
}

void Object::singleton_method_alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ClassObject *klass = singleton_class(env);
    if (klass->is_frozen())
        env->raise("FrozenError", "can't modify frozen object: {}", to_s(env)->string());
    klass->method_alias(env, new_name, old_name);
}

nat_int_t Object::object_id() const {
    if (is_integer()) {
        const auto i = as_integer();
        if (i->is_fixnum()) {
            /* Recreate the logic from Ruby: Use a long as tagged pointer, where
             * the rightmost bit is 1, and the remaning bits are the number shifted
             * one right.
             * The regular object ids are the actual memory addresses, these are at
             * least 8 bit aligned, so the rightmost bit will never be set. This
             * means we don't risk duplicate object ids for different objects.
             */
            auto val = i->to_nat_int_t();
            if (val >= (LONG_MIN >> 1) && val <= (LONG_MAX >> 1))
                return (val << 1) | 1;
        }
    }

    return reinterpret_cast<nat_int_t>(this);
}

SymbolObject *Object::define_singleton_method(Env *env, SymbolObject *name, MethodFnPtr fn, int arity, bool optimized) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ClassObject *klass = singleton_class(env);
    if (klass->is_frozen())
        env->raise("FrozenError", "can't modify frozen object: {}", to_s(env)->string());
    klass->define_method(env, name, fn, arity, optimized);
    return name;
}

SymbolObject *Object::define_singleton_method(Env *env, SymbolObject *name, Block *block) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ClassObject *klass = singleton_class(env);
    if (klass->is_frozen())
        env->raise("FrozenError", "can't modify frozen object: {}", to_s(env)->string());
    klass->define_method(env, name, block);
    return name;
}

SymbolObject *Object::undefine_singleton_method(Env *env, SymbolObject *name) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    ClassObject *klass = singleton_class(env);
    klass->undefine_method(env, name);
    return name;
}

SymbolObject *Object::define_method(Env *env, SymbolObject *name, MethodFnPtr fn, int arity, bool optimized) {
    m_klass->define_method(env, name, fn, arity, optimized);
    return name;
}

SymbolObject *Object::define_method(Env *env, SymbolObject *name, Block *block) {
    m_klass->define_method(env, name, block);
    return name;
}

SymbolObject *Object::undefine_method(Env *env, SymbolObject *name) {
    m_klass->undefine_method(env, name);
    return name;
}

Value Object::main_obj_define_method(Env *env, Value name, Value proc_or_unbound_method, Block *block) {
    return m_klass->define_method(env, name, proc_or_unbound_method, block);
}

void Object::private_method(Env *env, SymbolObject *name) {
    private_method(env, Args { name });
}

void Object::protected_method(Env *env, SymbolObject *name) {
    protected_method(env, Args { name });
}

void Object::module_function(Env *env, SymbolObject *name) {
    module_function(env, Args { name });
}

Value Object::private_method(Env *env, Args args) {
    if (!is_main_object()) {
        printf("tried to call private_method on something that has no methods\n");
        abort();
    }
    return m_klass->private_method(env, args);
}

Value Object::protected_method(Env *env, Args args) {
    if (!is_main_object()) {
        printf("tried to call protected_method on something that has no methods\n");
        abort();
    }
    return m_klass->protected_method(env, args);
}

Value Object::module_function(Env *env, Args args) {
    printf("tried to call module_function on something that isn't a module\n");
    abort();
}

Value Object::public_send(Env *env, SymbolObject *name, Args args, Block *block, Value sent_from) {
    return send(env, name, args, block, MethodVisibility::Public, sent_from);
}

Value Object::public_send(Env *env, Args args, Block *block) {
    auto name = args[0]->to_symbol(env, Object::Conversion::Strict);
    return public_send(env->caller(), name, Args::shift(args), block);
}

Value Object::send(Env *env, SymbolObject *name, Args args, Block *block, Value sent_from) {
    return send(env, name, args, block, MethodVisibility::Private, sent_from);
}

Value Object::send(Env *env, Args args, Block *block) {
    auto name = args[0]->to_symbol(env, Object::Conversion::Strict);
    return send(env->caller(), name, Args::shift(args), block);
}

Value Object::send(Env *env, SymbolObject *name, Args args, Block *block, MethodVisibility visibility_at_least, Value sent_from) {
    static const auto initialize = SymbolObject::intern("initialize");
    Method *method = find_method(env, name, visibility_at_least, sent_from);
    args.pop_empty_keyword_hash();
    if (method) {
        auto result = method->call(env, this, args, block);
        if (name == initialize)
            result = this;
        return result;
    } else if (respond_to(env, "method_missing"_s)) {
        return method_missing_send(env, name, args, block);
    } else {
        env->raise_no_method_error(this, name, GlobalEnv::the()->method_missing_reason());
    }
}

Value Object::method_missing_send(Env *env, SymbolObject *name, Args args, Block *block) {
    Vector<Value> new_args(args.size() + 1);
    new_args.push(name);
    for (size_t i = 0; i < args.size(); i++)
        new_args.push(args[i]);
    return send(env, "method_missing"_s, Args(new_args, args.has_keyword_hash()), block);
}

Value Object::method_missing(Env *env, Args args, Block *block) {
    if (args.size() == 0) {
        env->raise("ArgError", "no method name given");
    } else if (!args[0]->is_symbol()) {
        env->raise("ArgError", "method name must be a Symbol but {} is given", args[0]->klass()->inspect_str());
    } else {
        auto name = args[0]->as_symbol();
        env = env->caller();
        env->raise_no_method_error(this, name, GlobalEnv::the()->method_missing_reason());
    }
}

Method *Object::find_method(Env *env, SymbolObject *method_name, MethodVisibility visibility_at_least, Value sent_from) const {
    ModuleObject *klass = singleton_class();
    if (!klass)
        klass = m_klass;
    assert(klass);
    auto method_info = klass->find_method(env, method_name);

    if (!method_info.is_defined()) {
        // FIXME: store on current thread
        GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Undefined);
        return nullptr;
    }

    MethodVisibility visibility = method_info.visibility();

    if (visibility >= visibility_at_least)
        return method_info.method();

    if (visibility == MethodVisibility::Protected && sent_from && sent_from->is_a(env, klass))
        return method_info.method();

    switch (visibility) {
    case MethodVisibility::Protected:
        // FIXME: store on current thread
        GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Protected);
        break;
    case MethodVisibility::Private:
        // FIXME: store on current thread
        GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Private);
        break;
    default:
        NAT_UNREACHABLE();
    }

    return nullptr;
}

Value Object::dup(Env *env) const {
    switch (m_type) {
    case Object::Type::Array:
        return new ArrayObject { *as_array() };
    case Object::Type::Class: {
        auto out = new ClassObject { *as_class() };
        auto s_class = singleton_class();
        if (s_class)
            out->set_singleton_class(s_class->clone(env)->as_class());
        return out;
    }
    case Object::Type::Complex:
        return new ComplexObject { *as_complex() };
    case Object::Type::Exception:
        return new ExceptionObject { *as_exception() };
    case Object::Type::False:
        return FalseObject::the();
    case Object::Type::Float:
        return Value::floatingpoint(as_float()->to_double());
    case Object::Type::Hash:
        return new HashObject { env, *as_hash() };
    case Object::Type::Integer:
        if (as_integer()->is_bignum())
            return new IntegerObject { *as_integer() };
        return Value::integer(as_integer()->to_nat_int_t());
    case Object::Type::Module:
        return new ModuleObject { *as_module() };
    case Object::Type::Nil:
        return NilObject::the();
    case Object::Type::Object:
        return new Object { *this };
    case Object::Type::Proc:
        return new ProcObject { *as_proc() };
    case Object::Type::Range:
        return new RangeObject { *as_range() };
    case Object::Type::Rational:
        return new RationalObject { *as_rational() };
    case Object::Type::Regexp:
        return new RegexpObject { env, as_regexp()->pattern(), as_regexp()->options() };
    case Object::Type::String:
        return new StringObject { *as_string() };
    case Object::Type::Symbol:
        return SymbolObject::intern(as_symbol()->string());
    case Object::Type::True:
        return TrueObject::the();
    case Object::Type::UnboundMethod:
        return new UnboundMethodObject { *as_unbound_method() };
    case Object::Type::MatchData:
        return new MatchDataObject { *as_match_data() };
    default:
        fprintf(stderr, "I don't know how to dup this kind of object yet %s (type = %d).\n", m_klass->inspect_str().c_str(), static_cast<int>(m_type));
        abort();
    }
}

Value Object::clone(Env *env) {
    return this->clone(env, nullptr);
}

Value Object::clone(Env *env, Value freeze) {
    bool freeze_bool = true;
    if (freeze) {
        if (freeze->is_false()) {
            freeze_bool = false;
        } else if (!freeze->is_true() && !freeze->is_nil()) {
            env->raise("ArgumentError", "unexpected value for freeze: {}", freeze->klass()->inspect_str());
        }
    }

    auto duplicate = this->dup(env);
    if (!duplicate->singleton_class()) {
        auto s_class = singleton_class();
        if (s_class) {
            duplicate->set_singleton_class(s_class->clone(env)->as_class());
        }
    }

    if (freeze) {
        auto keyword_hash = new HashObject {};
        keyword_hash->put(env, "freeze"_s, freeze);
        auto args = Args({ this, keyword_hash }, true);
        duplicate->send(env, "initialize_clone"_s, args);
    } else {
        duplicate->send(env, "initialize_clone"_s, { this });
    }

    if (freeze_bool && is_frozen())
        duplicate->freeze();

    return duplicate;
}

bool Object::is_a(Env *env, Value val) const {
    if (!val->is_module()) return false;
    ModuleObject *module = val->as_module();
    if (m_klass == module || singleton_class() == module) {
        return true;
    } else {
        ClassObject *klass = singleton_class();
        if (!klass)
            klass = m_klass;
        return klass->ancestors_includes(env, module);
    }
}

bool Object::respond_to(Env *env, Value name_val, bool include_all) {
    if (respond_to_method(env, "respond_to?"_s, true)) {
        Value include_all_val;
        if (include_all) {
            include_all_val = TrueObject::the();
        } else {
            include_all_val = FalseObject::the();
        }
        return send(env, "respond_to?"_s, { name_val, include_all_val })->is_truthy();
    }

    // Needed for BaseObject as it does not have an actual respond_to? method
    return respond_to_method(env, name_val, include_all);
}

bool Object::respond_to_method(Env *env, Value name_val, bool include_all) {
    auto name_symbol = name_val->to_symbol(env, Conversion::Strict);

    ClassObject *klass = singleton_class();
    if (!klass)
        klass = m_klass;

    auto method_info = klass->find_method(env, name_symbol);
    if (!method_info.is_defined()) {
        if (klass->find_method(env, "respond_to_missing?"_s).is_defined()) {
            return send(env, "respond_to_missing?"_s, { name_val, bool_object(include_all) })->is_truthy();
        }
        return false;
    }

    if (include_all)
        return true;

    MethodVisibility visibility = method_info.visibility();
    if (visibility == MethodVisibility::Public) {
        return true;
    } else if (klass->find_method(env, "respond_to_missing?"_s).is_defined()) {
        return send(env, "respond_to_missing?"_s, { name_val, bool_object(include_all) })->is_truthy();
    } else {
        return false;
    }
}

bool Object::respond_to_method(Env *env, Value name_val, Value include_all_val) {
    bool include_all = include_all_val ? include_all_val->is_truthy() : false;
    return respond_to_method(env, name_val, include_all);
}

bool Object::respond_to_missing(Env *, Value, Value) {
    return false;
}

const char *Object::defined(Env *env, SymbolObject *name, bool strict) {
    Value obj = nullptr;
    if (name->is_constant_name()) {
        if (strict) {
            if (is_module()) {
                obj = as_module()->const_get(name);
            }
        } else {
            obj = const_find(env, name, ConstLookupSearchMode::NotStrict, ConstLookupFailureMode::Null);
        }
        if (obj) return "constant";
    } else if (name->is_global_name()) {
        obj = env->global_get(name);
        if (obj != NilObject::the()) return "global-variable";
    } else if (name->is_ivar_name()) {
        obj = ivar_get(env, name);
        if (obj != NilObject::the()) return "instance-variable";
    } else if (respond_to(env, name)) {
        return "method";
    }
    return nullptr;
}

Value Object::defined_obj(Env *env, SymbolObject *name, bool strict) {
    const char *result = defined(env, name, strict);
    if (result) {
        return new StringObject { result };
    } else {
        return NilObject::the();
    }
}

ProcObject *Object::to_proc(Env *env) {
    auto to_proc_symbol = "to_proc"_s;
    if (respond_to(env, to_proc_symbol)) {
        return send(env, to_proc_symbol)->as_proc();
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Proc)", m_klass->inspect_str());
    }
}

void Object::freeze() {
    m_flags = m_flags | Flag::Frozen;
    if (m_singleton_class) m_singleton_class->freeze();
}

Value Object::instance_eval(Env *env, Value string, Block *block) {
    if (string || !block) {
        env->raise("ArgumentError", "Natalie only supports instance_eval with a block");
    }
    Value self = this;
    block->set_self(self);
    GlobalEnv::the()->set_instance_evaling(true);
    Defer done_instance_evaling([]() { GlobalEnv::the()->set_instance_evaling(false); });
    Value args[] = { self };
    return NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
}

void Object::assert_type(Env *env, Object::Type expected_type, const char *expected_class_name) const {
    if ((type()) != expected_type) {
        if (type() == ObjectType::Nil) {
            // For some reason the errors with nil are slightly different...
            char first_letter = std::tolower(expected_class_name[0]);
            char const beginning[] = { first_letter, '\0' };
            env->raise("TypeError", "no implicit conversion from nil to {}{}", beginning, expected_class_name + 1);
        } else {
            env->raise("TypeError", "no implicit conversion of {} into {}", klass()->inspect_str(), expected_class_name);
        }
    }
}

void Object::assert_not_frozen(Env *env) {
    if (is_frozen()) {
        env->raise("FrozenError", "can't modify frozen {}: {}", klass()->inspect_str(), inspect_str(env));
    }
}

void Object::assert_not_frozen(Env *env, Value receiver) {
    if (is_frozen()) {
        auto FrozenError = GlobalEnv::the()->Object()->const_fetch("FrozenError"_s);
        String message = String::format("can't modify frozen {}: {}", klass()->inspect_str(), inspect_str(env));
        auto kwargs = new HashObject(env, { "receiver"_s, receiver });
        auto args = Args({ new StringObject { message }, kwargs }, true);
        ExceptionObject *error = FrozenError.send(env, "new"_s, args)->as_exception();
        env->raise_exception(error);
    }
}

bool Object::equal(Value other) {
    if (is_integer() && as_integer()->is_fixnum() && other->is_integer() && other->as_integer()->is_fixnum())
        return as_integer()->to_nat_int_t() == other->as_integer()->to_nat_int_t();
    // We still need the pointer compare for the identical NaN equality
    if (is_float() && other->is_float())
        return this == other.object() || as_float()->to_double() == other->as_float()->to_double();

    return other == this;
}

bool Object::neq(Env *env, Value other) {
    return send(env, "=="_s, { other })->is_falsey();
}

String Object::dbg_inspect() const {
    auto klass = m_klass->class_name();
    return String::format(
        "#<{}:{}>",
        klass ? *klass : "Object",
        String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed));
}

String Object::inspect_str(Env *env) {
    if (!respond_to(env, "inspect"_s))
        return String::format("#<{}:{}>", m_klass->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed));
    auto inspected = send(env, "inspect"_s);
    if (!inspected->is_string())
        return ""; // TODO: what to do here?
    return inspected->as_string()->string();
}

Value Object::enum_for(Env *env, const char *method, Args args) {
    Vector<Value> args2(args.size() + 1);
    args2.push(SymbolObject::intern(method));
    for (size_t i = 0; i < args.size(); i++) {
        args2.push(args[i]);
    }
    return this->public_send(env, "enum_for"_s, Args(std::move(args2), args.has_keyword_hash()));
}

void Object::visit_children(Visitor &visitor) {
    visitor.visit(m_klass);
    visitor.visit(m_singleton_class);
    visitor.visit(m_owner);
    if (m_ivars) {
        for (auto pair : *m_ivars) {
            visitor.visit(pair.first);
            visitor.visit(pair.second);
        }
    }
}

void Object::gc_inspect(char *buf, size_t len) const {
    snprintf(buf, len, "<Object %p type=%d class=%p>", this, (int)m_type, m_klass);
}

ArrayObject *Object::to_ary(Env *env) {
    if (is_array()) {
        return as_array();
    }

    auto original_class = klass()->inspect_str();

    auto to_ary = "to_ary"_s;

    if (!respond_to(env, to_ary)) {
        if (is_nil()) {
            env->raise("TypeError", "no implicit conversion of nil into Array");
        }
        env->raise("TypeError", "no implicit conversion of {} into Array", original_class);
    }

    Value val = send(env, to_ary);

    if (val->is_array()) {
        return val->as_array();
    }

    env->raise(
        "TypeError", "can't convert {} to Array ({}#to_ary gives {})",
        original_class,
        original_class,
        val->klass()->inspect_str());
}

IoObject *Object::to_io(Env *env) {
    if (is_io()) return as_io();

    auto to_io = "to_io"_s;
    if (!respond_to(env, to_io)) {
        assert_type(env, Type::Io, "IO");
    }

    auto result = send(env, to_io);

    if (result->is_io())
        return result->as_io();

    env->raise(
        "TypeError", "can't convert {} to IO ({}#to_io gives {})",
        klass()->inspect_str(),
        klass()->inspect_str(),
        result->klass()->inspect_str());
}

IntegerObject *Object::to_int(Env *env) {
    if (is_integer()) return as_integer();

    auto to_int = "to_int"_s;
    if (!respond_to(env, to_int)) {
        assert_type(env, Type::Integer, "Integer");
    }

    auto result = send(env, to_int);

    if (result->is_integer())
        return result->as_integer();

    env->raise(
        "TypeError", "can't convert {} to Integer ({}#to_int gives {})",
        klass()->inspect_str(),
        klass()->inspect_str(),
        result->klass()->inspect_str());
}

FloatObject *Object::to_f(Env *env) {
    if (is_float()) return as_float();

    auto to_f = "to_f"_s;
    if (!respond_to(env, to_f))
        assert_type(env, Type::Float, "Float");

    auto result = send(env, to_f);
    result->assert_type(env, Type::Float, "Float");
    return result->as_float();
}

HashObject *Object::to_hash(Env *env) {
    if (is_hash()) {
        return as_hash();
    }

    auto original_class = klass()->inspect_str();

    auto to_hash = "to_hash"_s;

    if (!respond_to(env, to_hash)) {
        if (is_nil()) {
            env->raise("TypeError", "no implicit conversion of nil into Hash");
        }
        env->raise("TypeError", "no implicit conversion of {} into Hash", original_class);
    }

    Value val = send(env, to_hash);

    if (val->is_hash()) {
        return val->as_hash();
    }

    env->raise(
        "TypeError", "can't convert {} to Hash ({}#to_hash gives {})",
        original_class,
        original_class,
        val->klass()->inspect_str());
}

StringObject *Object::to_s(Env *env) {
    auto str = send(env, "to_s"_s);
    if (!str->is_string())
        env->raise("TypeError", "no implicit conversion of {} into String", m_klass->class_name());
    return str->as_string();
}

StringObject *Object::to_str(Env *env) {
    if (is_string()) return as_string();

    auto to_str = "to_str"_s;
    if (!respond_to(env, to_str)) {
        assert_type(env, Type::String, "String");
    }

    auto result = send(env, to_str);

    if (result->is_string())
        return result->as_string();

    env->raise(
        "TypeError", "can't convert {} to String ({}#to_str gives {})",
        klass()->inspect_str(),
        klass()->inspect_str(),
        result->klass()->inspect_str());
}

}
