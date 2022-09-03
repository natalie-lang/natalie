#include "natalie.hpp"
#include "natalie/forward.hpp"
#include <cctype>

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
    case Object::Type::Array:
        obj = new ArrayObject {};
        obj->m_klass = klass;
        break;

    case Object::Type::Class:
        obj = new ClassObject { klass };
        break;

    case Object::Type::Encoding:
        obj = new EncodingObject { klass };
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

    case Object::Type::VoidP:
        obj = new VoidPObject { klass };
        break;

    case Object::Type::Binding:
    case Object::Type::Method:
    case Object::Type::Nil:
    case Object::Type::False:
    case Object::Type::True:
    case Object::Type::Integer:
    case Object::Type::Float:
    case Object::Type::Rational:
    case Object::Type::Symbol:
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

ArrayObject *Object::as_array() {
    assert(is_array());
    return static_cast<ArrayObject *>(this);
}

MethodObject *Object::as_method() {
    assert(is_method());
    return static_cast<MethodObject *>(this);
}

ModuleObject *Object::as_module() {
    assert(is_module());
    return static_cast<ModuleObject *>(this);
}

ClassObject *Object::as_class() {
    assert(is_class());
    return static_cast<ClassObject *>(this);
}

EncodingObject *Object::as_encoding() {
    assert(is_encoding());
    return static_cast<EncodingObject *>(this);
}

ExceptionObject *Object::as_exception() {
    assert(is_exception());
    return static_cast<ExceptionObject *>(this);
}

FalseObject *Object::as_false() {
    assert(is_false());
    return static_cast<FalseObject *>(this);
}

FiberObject *Object::as_fiber() {
    assert(is_fiber());
    return static_cast<FiberObject *>(this);
}

FloatObject *Object::as_float() {
    assert(is_float());
    return static_cast<FloatObject *>(this);
}

HashObject *Object::as_hash() {
    assert(is_hash());
    return static_cast<HashObject *>(this);
}

IntegerObject *Object::as_integer() {
    assert(is_integer());
    return static_cast<IntegerObject *>(this);
}

IoObject *Object::as_io() {
    assert(is_io());
    return static_cast<IoObject *>(this);
}

FileObject *Object::as_file() {
    assert(is_io());
    return static_cast<FileObject *>(this);
}

MatchDataObject *Object::as_match_data() {
    assert(is_match_data());
    return static_cast<MatchDataObject *>(this);
}

ProcObject *Object::as_proc() {
    assert(is_proc());
    return static_cast<ProcObject *>(this);
}

RandomObject *Object::as_random() {
    assert(is_random());
    return static_cast<RandomObject *>(this);
}

RangeObject *Object::as_range() {
    assert(is_range());
    return static_cast<RangeObject *>(this);
}

RationalObject *Object::as_rational() {
    assert(is_rational());
    return static_cast<RationalObject *>(this);
}

RegexpObject *Object::as_regexp() {
    assert(is_regexp());
    return static_cast<RegexpObject *>(this);
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

TrueObject *Object::as_true() {
    assert(is_true());
    return static_cast<TrueObject *>(this);
}

UnboundMethodObject *Object::as_unbound_method() {
    assert(is_unbound_method());
    return static_cast<UnboundMethodObject *>(this);
}

VoidPObject *Object::as_void_p() {
    assert(is_void_p());
    return static_cast<VoidPObject *>(this);
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

SexpObject *Object::as_sexp_object_for_method_binding() {
    return static_cast<SexpObject *>(this);
}

SymbolObject *Object::to_symbol(Env *env, Conversion conversion) {
    if (is_symbol()) {
        return as_symbol();
    } else if (is_string()) {
        return as_string()->to_symbol(env);
    } else if (respond_to(env, "to_str"_s)) {
        Value str = send(env, "to_str"_s);
        if (str->is_string()) {
            return str->as_string()->to_symbol(env);
        } else {
            auto *class_name = klass()->inspect_str();
            env->raise("TypeError", "can't convert {} to String ({}#to_str gives {})", class_name, class_name, str->klass()->inspect_str());
        }
    } else if (conversion == Conversion::NullAllowed) {
        return nullptr;
    } else {
        env->raise("TypeError", "{} is not a symbol nor a string", inspect_str(env));
    }
}

SymbolObject *Object::to_instance_variable_name(Env *env) {
    SymbolObject *symbol = to_symbol(env, Conversion::Strict);

    if (!symbol->is_ivar_name()) {
        env->raise_name_error(symbol, "`{}' is not allowed as an instance variable name", symbol->c_str());
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

    const ManagedString *inspect_string = nullptr;
    if (is_module()) {
        inspect_string = as_module()->inspect_str();
    } else if (respond_to(env, "inspect"_s)) {
        inspect_string = inspect_str(env);
    }
    const ManagedString *name = nullptr;
    if (inspect_string) {
        name = ManagedString::format("#<Class:{}>", inspect_string);
    }

    ClassObject *singleton_superclass;
    if (is_class()) {
        singleton_superclass = as_class()->superclass(env)->singleton_class(env);
    } else {
        singleton_superclass = m_klass;
    }
    auto new_singleton_class = new ClassObject { singleton_superclass };
    singleton_superclass->initialize_subclass_without_checks(new_singleton_class, env, name);
    set_singleton_class(new_singleton_class);
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

Value Object::const_get(SymbolObject *name) const {
    return m_klass->const_get(name);
}

Value Object::const_fetch(SymbolObject *name) {
    return m_klass->const_fetch(name);
}

Value Object::const_set(SymbolObject *name, Value val) {
    return m_klass->const_set(name, val);
}

bool Object::ivar_defined(Env *env, SymbolObject *name) {
    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->c_str());

    if (!m_ivars)
        return false;

    auto val = m_ivars->get(name, env);
    if (val)
        return true;
    else
        return false;
}

Value Object::ivar_get(Env *env, SymbolObject *name) {
    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->c_str());

    if (!m_ivars)
        return NilObject::the();

    auto val = m_ivars->get(name, env);
    if (val)
        return val;
    else
        return NilObject::the();
}

Value Object::ivar_remove(Env *env, SymbolObject *name) {
    if (!name->is_ivar_name())
        env->raise("NameError", "`{}' is not allowed as an instance variable name", name->c_str());

    if (!m_ivars)
        env->raise("NameError", "instance variable {} not defined", name->c_str());

    auto val = m_ivars->remove(name, env);
    if (val)
        return val;
    else
        env->raise("NameError", "instance variable {} not defined", name->c_str());
}

Value Object::ivar_set(Env *env, SymbolObject *name, Value val) {
    NAT_GC_GUARD_VALUE(val);

    assert_not_frozen(env);

    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->c_str());

    if (!m_ivars)
        m_ivars = new TM::Hashmap<SymbolObject *, Value> {};

    m_ivars->put(name, val.object(), env);
    return val;
}

Value Object::instance_variables(Env *env) {
    if (m_type == Object::Type::Integer || m_type == Object::Type::Float) {
        return new ArrayObject;
    }

    ArrayObject *ary = new ArrayObject { m_ivars->size() };

    if (!m_ivars)
        return ary;

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
        env->raise_name_error(name, "uninitialized class variable {} in {}", name->c_str(), module->inspect_str());
    }
}

Value Object::cvar_get_or_null(Env *env, SymbolObject *name) {
    return m_klass->cvar_get_or_null(env, name);
}

Value Object::cvar_set(Env *env, SymbolObject *name, Value val) {
    return m_klass->cvar_set(env, name, val);
}

void Object::alias(Env *env, Value new_name, Value old_name) {
    new_name->assert_type(env, Object::Type::Symbol, "Symbol");
    old_name->assert_type(env, Object::Type::Symbol, "Symbol");
    alias(env, static_cast<SymbolObject *>(new_name.object()), static_cast<SymbolObject *>(old_name.object()));
}

void Object::alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    if (is_integer() || is_symbol()) {
        env->raise("TypeError", "no klass to make alias");
    }
    if (is_main_object()) {
        m_klass->make_alias(env, new_name, old_name);
    } else if (is_module()) {
        as_module()->alias(env, new_name, old_name);
    } else {
        singleton_class(env)->make_alias(env, new_name, old_name);
    }
}

SymbolObject *Object::define_singleton_method(Env *env, SymbolObject *name, MethodFnPtr fn, int arity, bool optimized) {
    ClassObject *klass = singleton_class(env);
    klass->define_method(env, name, fn, arity, optimized);
    return name;
}

SymbolObject *Object::define_singleton_method(Env *env, SymbolObject *name, Block *block) {
    ClassObject *klass = singleton_class(env);
    klass->define_method(env, name, block);
    return name;
}

SymbolObject *Object::undefine_singleton_method(Env *env, SymbolObject *name) {
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
    Value args[] = { name };
    private_method(env, Args(1, args));
}

void Object::protected_method(Env *env, SymbolObject *name) {
    Value args[] = { name };
    protected_method(env, Args(1, args));
}

void Object::module_function(Env *env, SymbolObject *name) {
    Value args[] = { name };
    module_function(env, Args(1, args));
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

Value Object::public_send(Env *env, SymbolObject *name, Args args, Block *block) {
    return send(env, name, args, block, MethodVisibility::Public);
}

Value Object::public_send(Env *env, Args args, Block *block) {
    auto name = args[0]->to_symbol(env, Object::Conversion::Strict);
    return public_send(env->caller(), name, Args::shift(args), block);
}

Value Object::send(Env *env, SymbolObject *name, Args args, Block *block) {
    return send(env, name, args, block, MethodVisibility::Private);
}

Value Object::send(Env *env, Args args, Block *block) {
    auto name = args[0]->to_symbol(env, Object::Conversion::Strict);
    return send(env->caller(), name, Args::shift(args), block);
}

Value Object::send(Env *env, SymbolObject *name, Args args, Block *block, MethodVisibility visibility_at_least) {
    Method *method = find_method(env, name, visibility_at_least);
    if (method) {
        return method->call(env, this, args, block);
    } else if (respond_to(env, "method_missing"_s)) {
        ArrayObject new_args { args.size() + 1 };
        new_args.push(name);
        new_args.push(env, args);
        return send(env, "method_missing"_s, Args(&new_args), block);
    } else {
        env->raise_no_method_error(this, name, GlobalEnv::the()->method_missing_reason());
    }
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

Method *Object::find_method(Env *env, SymbolObject *method_name, MethodVisibility visibility_at_least) {
    ModuleObject *klass = singleton_class();
    if (!klass)
        klass = m_klass;
    auto method_info = klass->find_method(env, method_name);
    if (method_info.is_defined()) {
        MethodVisibility visibility = method_info.visibility();
        if (visibility >= visibility_at_least) {
            return method_info.method();
        } else if (visibility == MethodVisibility::Protected) {
            GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Protected);
        } else {
            GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Private);
        }
    } else {
        GlobalEnv::the()->set_method_missing_reason(MethodMissingReason::Undefined);
    }
    return nullptr;
}

Value Object::dup(Env *env) {
    switch (m_type) {
    case Object::Type::Array:
        return new ArrayObject { *as_array() };
    case Object::Type::Class:
        return new ClassObject { *as_class() };
    case Object::Type::Exception:
        return new ExceptionObject { *as_exception() };
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
    case Object::Type::Proc:
        return new ProcObject { *as_proc() };
    case Object::Type::Range:
        return new RangeObject { *as_range() };
    case Object::Type::String:
        return new StringObject { *as_string() };
    case Object::Type::False:
    case Object::Type::Nil:
    case Object::Type::Rational:
    case Object::Type::Symbol:
    case Object::Type::True:
        return this;
    case Object::Type::Object:
        return new Object { *this };
    default:
        fprintf(stderr, "I don't know how to dup this kind of object yet %s (type = %d).\n", m_klass->inspect_str()->c_str(), static_cast<int>(m_type));
        abort();
    }
}

Value Object::clone(Env *env) {
    auto duplicate = this->dup(env);
    auto s_class = singleton_class();
    if (s_class) {
        duplicate->set_singleton_class(s_class->clone(env)->as_class());
    }
    return duplicate;
}

bool Object::is_a(Env *env, Value val) const {
    if (!val->is_module()) return false;
    ModuleObject *module = val->as_module();
    if (this == module) {
        return false;
    } else if (m_klass == module || singleton_class() == module) {
        return true;
    } else {
        ClassObject *klass = singleton_class();
        if (!klass)
            klass = m_klass;
        ArrayObject *ancestors = klass->ancestors(env);
        for (Value m : *ancestors) {
            if (module == m->as_module()) {
                return true;
            }
        }
        return false;
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

bool Object::respond_to_method(Env *env, Value name_val, bool include_all) const {
    auto name_symbol = name_val->to_symbol(env, Conversion::Strict);

    ClassObject *klass = singleton_class();
    if (!klass)
        klass = m_klass;

    auto method_info = klass->find_method(env, name_symbol);
    if (!method_info.is_defined())
        return false;

    if (include_all)
        return true;

    MethodVisibility visibility = method_info.visibility();
    return visibility == MethodVisibility::Public;
}

bool Object::respond_to_method(Env *env, Value name_val, Value include_all_val) const {
    bool include_all = include_all_val ? include_all_val->is_truthy() : false;
    return respond_to_method(env, name_val, include_all);
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

void Object::assert_type(Env *env, Object::Type expected_type, const char *expected_class_name) {
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

bool Object::equal(Value other) {
    if (is_integer() && other->is_integer())
        return as_integer()->to_nat_int_t() == other->as_integer()->to_nat_int_t();
    // We still need the pointer compare for the identical NaN equality
    if (is_float() && other->is_float())
        return this == other.object() || as_float()->to_double() == other->as_float()->to_double();

    return other == this;
}

bool Object::neq(Env *env, Value other) {
    return send(env, "=="_s, { other })->is_falsey();
}

const ManagedString *Object::inspect_str(Env *env) {
    if (!respond_to(env, "inspect"_s))
        return ManagedString::format("#<{}:{}>", m_klass->inspect_str(), String::hex(object_id(), String::HexFormat::LowercaseAndPrefixed));
    auto inspected = send(env, "inspect"_s);
    if (!inspected->is_string())
        return new ManagedString(""); // TODO: what to do here?
    return inspected->as_string()->to_low_level_string();
}

Value Object::enum_for(Env *env, const char *method, Args args) {
    Value args2[args.size() + 1];
    args2[0] = SymbolObject::intern(method);
    for (size_t i = 0; i < args.size(); i++) {
        args2[i + 1] = args[i];
    }
    return this->public_send(env, "enum_for"_s, Args(args.size() + 1, args2));
}

void Object::visit_children(Visitor &visitor) {
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
