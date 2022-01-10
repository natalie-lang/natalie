#include "natalie.hpp"
#include "natalie/forward.hpp"
#include <cctype>

namespace Natalie {

Object::Object(const Object &other)
    : m_klass { other.m_klass }
    , m_type { other.m_type }
    , m_singleton_class { other.m_singleton_class ? new ClassObject { other.m_singleton_class } : nullptr }
    , m_owner { other.m_owner }
    , m_ivars { other.m_ivars } { }

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
    case Object::Type::Symbol:
    case Object::Type::UnboundMethod:
        obj = nullptr;
        break;
    }

    return obj;
}

Value Object::_new(Env *env, Value klass_value, size_t argc, Value *args, Block *block) {
    Value obj = create(env, klass_value->as_class());
    if (!obj)
        NAT_UNREACHABLE();

    return obj->initialize(env, argc, args, block);
}

Value Object::allocate(Env *env, Value klass_value, size_t argc, Value *args, Block *block) {
    env->ensure_argc_is(argc, 0);

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

Value Object::initialize(Env *env, size_t argc, Value *args, Block *block) {
    Method *method = m_klass->find_method(env, "initialize"_s);
    if (method && !method->is_undefined()) {
        method->call(env, this, argc, args, block);
    } else {
        env->ensure_argc_is(argc, 0);
    }
    return this;
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

const String *Object::identifier_str(Env *env, Conversion conversion) {
    if (is_symbol()) {
        return as_symbol()->to_s(env)->to_low_level_string();
    } else if (is_string()) {
        return as_string()->to_low_level_string();
    } else if (conversion == Conversion::NullAllowed) {
        return nullptr;
    } else {
        env->raise("TypeError", "{} is not a symbol nor a string", inspect_str(env));
    }
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

    const String *inspect_string = nullptr;
    if (is_module()) {
        inspect_string = as_module()->inspect_str();
    } else if (respond_to(env, "inspect"_s)) {
        inspect_string = inspect_str(env);
    }
    const String *name = nullptr;
    if (inspect_string) {
        name = String::format("#<Class:{}>", inspect_string);
    }

    ClassObject *singleton_superclass;
    if (is_class()) {
        singleton_superclass = as_class()->superclass(env)->singleton_class(env);
    } else {
        singleton_superclass = m_klass;
    }
    set_singleton_class(singleton_superclass->subclass(env, name));
    return m_singleton_class;
}

Value Object::extend(Env *env, size_t argc, Value *args) {
    assert_not_frozen(env);
    for (size_t i = 0; i < argc; i++) {
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

Value Object::const_get(SymbolObject *name) {
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

    auto val = m_ivars.get(name, env);
    if (val)
        return true;
    else
        return false;
}

Value Object::ivar_get(Env *env, SymbolObject *name) {
    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->c_str());

    auto val = m_ivars.get(name, env);
    if (val)
        return val;
    else
        return NilObject::the();
}

Value Object::ivar_remove(Env *env, SymbolObject *name) {
    if (!name->is_ivar_name())
        env->raise("NameError", "`{}' is not allowed as an instance variable name", name->c_str());

    auto val = m_ivars.remove(name, env);
    if (val)
        return val;
    else
        env->raise("NameError", "instance variable {} not defined", name->c_str());
}

Value Object::ivar_set(Env *env, SymbolObject *name, Value val) {
    if (!name->is_ivar_name())
        env->raise_name_error(name, "`{}' is not allowed as an instance variable name", name->c_str());

    m_ivars.put(name, val.object(), env);
    return val;
}

Value Object::instance_variables(Env *env) {
    if (m_type == Object::Type::Integer || m_type == Object::Type::Float) {
        return new ArrayObject;
    }
    ArrayObject *ary = new ArrayObject { m_ivars.size() };
    for (auto pair : m_ivars) {
        ary->push(pair.first);
    }
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

void Object::alias(Env *env, SymbolObject *new_name, SymbolObject *old_name) {
    if (is_integer() || is_symbol()) {
        env->raise("TypeError", "no klass to make alias");
    }
    if (is_main_object()) {
        m_klass->alias(env, new_name, old_name);
    } else if (is_module()) {
        as_module()->alias(env, new_name, old_name);
    } else {
        singleton_class(env)->alias(env, new_name, old_name);
    }
}

SymbolObject *Object::define_singleton_method(Env *env, SymbolObject *name, MethodFnPtr fn, int arity) {
    ClassObject *klass = singleton_class(env);
    klass->define_method(env, name, fn, arity);
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

SymbolObject *Object::define_method(Env *env, SymbolObject *name, MethodFnPtr fn, int arity) {
    if (!is_main_object()) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    m_klass->define_method(env, name, fn, arity);
    return name;
}

SymbolObject *Object::define_method(Env *env, SymbolObject *name, Block *block) {
    if (!is_main_object()) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    m_klass->define_method(env, name, block);
    return name;
}

SymbolObject *Object::undefine_method(Env *env, SymbolObject *name) {
    if (!is_main_object()) {
        printf("tried to call undefine_method on something that has no methods\n");
        abort();
    }
    m_klass->undefine_method(env, name);
    return name;
}

void Object::private_method(Env *env, SymbolObject *name) {
    Value args[] = { name };
    private_method(env, 1, args);
}

void Object::protected_method(Env *env, SymbolObject *name) {
    Value args[] = { name };
    protected_method(env, 1, args);
}

void Object::module_function(Env *env, SymbolObject *name) {
    Value args[] = { name };
    module_function(env, 1, args);
}

Value Object::private_method(Env *env, size_t argc, Value *args) {
    if (!is_main_object()) {
        printf("tried to call private_method on something that has no methods\n");
        abort();
    }
    return m_klass->private_method(env, argc, args);
}

Value Object::protected_method(Env *env, size_t argc, Value *args) {
    if (!is_main_object()) {
        printf("tried to call protected_method on something that has no methods\n");
        abort();
    }
    return m_klass->protected_method(env, argc, args);
}

Value Object::module_function(Env *env, size_t argc, Value *args) {
    printf("tried to call module_function on something that isn't a module\n");
    abort();
}

Value Object::public_send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    Method *method = find_method(env, name, MethodVisibility::Public);
    return method->call(env, this, argc, args, block);
}

Value Object::send(Env *env, SymbolObject *name, size_t argc, Value *args, Block *block) {
    Method *method = find_method(env, name, MethodVisibility::Private);
    return method->call(env, this, argc, args, block);
}

Value Object::send(Env *env, size_t argc, Value *args, Block *block) {
    auto name = args[0]->to_symbol(env, Object::Conversion::Strict);
    return send(env->caller(), name, argc - 1, args + 1, block);
}

Method *Object::find_method(Env *env, SymbolObject *method_name, MethodVisibility visibility_at_least) {
    auto singleton = singleton_class();
    if (singleton) {
        Method *method = singleton_class()->find_method(env, method_name);
        if (method) {
            if (!method->is_undefined()) {
                MethodVisibility visibility = singleton_class()->get_method_visibility(env, method_name);
                if (visibility >= visibility_at_least) {
                    return method;
                } else if (visibility == MethodVisibility::Protected) {
                    env->raise("NoMethodError", "protected method `{}' called for {}:Class", method_name->c_str(), m_klass->inspect_str());
                } else {
                    env->raise("NoMethodError", "private method `{}' called for {}:Class", method_name->c_str(), m_klass->inspect_str());
                }
            } else {
                env->raise("NoMethodError", "undefined method `{}' for {}:Class", method_name->c_str(), m_klass->inspect_str());
            }
        }
    }
    ModuleObject *klass = this->klass();
    Method *method = klass->find_method(env, method_name);
    if (method && !method->is_undefined()) {
        MethodVisibility visibility = klass->get_method_visibility(env, method_name);
        if (visibility >= visibility_at_least) {
            return method;
        } else if (visibility == MethodVisibility::Protected) {
            env->raise("NoMethodError", "protected method `{}' called for {}", method_name->c_str(), inspect_str(env));
        } else {
            env->raise("NoMethodError", "private method `{}' called for {}", method_name->c_str(), inspect_str(env));
        }
    } else if (method_name == "inspect"_s) {
        env->raise("NoMethodError", "undefined method `inspect' for #<{}:{}>", klass->inspect_str(), int_to_hex_string(object_id(), false));
    } else if (is_module()) {
        env->raise("NoMethodError", "undefined method `{}' for {}:{}", method_name->c_str(), as_module()->inspect_str(), klass->inspect_str());
    } else {
        env->raise("NoMethodError", "undefined method `{}' for {}", method_name->c_str(), inspect_str(env));
    }
}

Value Object::dup(Env *env) {
    switch (m_type) {
    case Object::Type::Array:
        return new ArrayObject { *as_array() };
    case Object::Type::Exception:
        return new ExceptionObject { *as_exception() };
    case Object::Type::Hash:
        return new HashObject { env, *as_hash() };
    case Object::Type::String:
        return new StringObject { *as_string() };
    case Object::Type::Range:
        return new RangeObject { *as_range() };
    case Object::Type::False:
    case Object::Type::Float:
    case Object::Type::Integer:
    case Object::Type::Nil:
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

    Method *method = klass->find_method(env, name_symbol);
    if (!method || method->is_undefined())
        return false;

    if (include_all)
        return true;

    MethodVisibility visibility = klass->get_method_visibility(env, name_symbol);
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
    if (is_module()) {
        // I *think* this is right... instance_eval, when called on a class/module,
        // evals with self set to the singleton class
        block->set_self(singleton_class(env));
    } else {
        block->set_self(this);
    }
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    return NilObject::the();
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

    return other == this;
}

bool Object::neq(Env *env, Value other) {
    return send(env, "=="_s, { other })->is_falsey();
}

const String *Object::inspect_str(Env *env) {
    auto inspected = send(env, "inspect"_s);
    if (!inspected->is_string())
        return new String(""); // TODO: what to do here?
    return inspected->as_string()->to_low_level_string();
}

Value Object::enum_for(Env *env, const char *method, size_t argc, Value *args) {
    Value args2[argc + 1];
    args2[0] = SymbolObject::intern(method);
    for (size_t i = 0; i < argc; i++) {
        args2[i + 1] = args[i];
    }
    return this->public_send(env, "enum_for"_s, argc + 1, args2);
}

void Object::visit_children(Visitor &visitor) {
    visitor.visit(m_singleton_class);
    visitor.visit(m_owner);
    for (auto pair : m_ivars) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
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

}
