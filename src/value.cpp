#include "natalie.hpp"
#include "natalie/forward.hpp"
#include <cctype>

namespace Natalie {

Value::Value(const Value &other)
    : m_klass { other.m_klass }
    , m_type { other.m_type }
    , m_singleton_class { other.m_singleton_class ? new ClassValue { other.m_singleton_class } : nullptr }
    , m_owner { other.m_owner }
    , m_ivars { other.m_ivars } { }

ValuePtr Value::_new(Env *env, ValuePtr klass_value, size_t argc, ValuePtr *args, Block *block) {
    ClassValue *klass = klass_value->as_class();
    ValuePtr obj;
    switch (klass->object_type()) {
    case Value::Type::Array:
        obj = new ArrayValue {};
        obj->m_klass = klass;
        break;

    case Value::Type::Class:
        obj = new ClassValue { klass };
        break;

    case Value::Type::Encoding:
        obj = new EncodingValue { klass };
        break;

    case Value::Type::Exception:
        obj = new ExceptionValue { klass };
        break;

    case Value::Type::Fiber:
        obj = new FiberValue { klass };
        break;

    case Value::Type::Hash:
        obj = new HashValue { klass };
        break;

    case Value::Type::Io:
        obj = new IoValue { klass };
        break;

    case Value::Type::MatchData:
        obj = new MatchDataValue { klass };
        break;

    case Value::Type::Module:
        obj = new ModuleValue { klass };
        break;

    case Value::Type::Object:
        obj = new Value { klass };
        break;

    case Value::Type::Proc:
        obj = new ProcValue { klass };
        break;

    case Value::Type::Range:
        obj = new RangeValue { klass };
        break;

    case Value::Type::Regexp:
        obj = new RegexpValue { klass };
        break;

    case Value::Type::String:
        obj = new StringValue { klass };
        break;

    case Value::Type::VoidP:
        obj = new VoidPValue { klass };
        break;

    case Value::Type::Binding:
    case Value::Type::Method:
    case Value::Type::Nil:
    case Value::Type::False:
    case Value::Type::True:
    case Value::Type::Integer:
    case Value::Type::Float:
    case Value::Type::Symbol:
        NAT_UNREACHABLE();
    }

    return obj->initialize(env, argc, args, block);
}

ValuePtr Value::initialize(Env *env, size_t argc, ValuePtr *args, Block *block) {
    Method *method = m_klass->find_method(env, SymbolValue::intern("initialize"));
    if (method && !method->is_undefined()) {
        method->call(env, this, argc, args, block);
    }
    return this;
}

NilValue *Value::as_nil() {
    assert(is_nil());
    return static_cast<NilValue *>(this);
}

ArrayValue *Value::as_array() {
    assert(is_array());
    return static_cast<ArrayValue *>(this);
}

MethodValue *Value::as_method() {
    assert(is_method());
    return static_cast<MethodValue *>(this);
}

ModuleValue *Value::as_module() {
    assert(is_module());
    return static_cast<ModuleValue *>(this);
}

ClassValue *Value::as_class() {
    assert(is_class());
    return static_cast<ClassValue *>(this);
}

EncodingValue *Value::as_encoding() {
    assert(is_encoding());
    return static_cast<EncodingValue *>(this);
}

ExceptionValue *Value::as_exception() {
    assert(is_exception());
    return static_cast<ExceptionValue *>(this);
}

FalseValue *Value::as_false() {
    assert(is_false());
    return static_cast<FalseValue *>(this);
}

FiberValue *Value::as_fiber() {
    assert(is_fiber());
    return static_cast<FiberValue *>(this);
}

FloatValue *Value::as_float() {
    assert(is_float());
    return static_cast<FloatValue *>(this);
}

HashValue *Value::as_hash() {
    assert(is_hash());
    return static_cast<HashValue *>(this);
}

IntegerValue *Value::as_integer() {
    assert(is_integer());
    return static_cast<IntegerValue *>(this);
}

IoValue *Value::as_io() {
    assert(is_io());
    return static_cast<IoValue *>(this);
}

FileValue *Value::as_file() {
    assert(is_io());
    return static_cast<FileValue *>(this);
}

MatchDataValue *Value::as_match_data() {
    assert(is_match_data());
    return static_cast<MatchDataValue *>(this);
}

ProcValue *Value::as_proc() {
    assert(is_proc());
    return static_cast<ProcValue *>(this);
}

RangeValue *Value::as_range() {
    assert(is_range());
    return static_cast<RangeValue *>(this);
}

RegexpValue *Value::as_regexp() {
    assert(is_regexp());
    return static_cast<RegexpValue *>(this);
}

StringValue *Value::as_string() {
    assert(is_string());
    return static_cast<StringValue *>(this);
}

const StringValue *Value::as_string() const {
    assert(is_string());
    return static_cast<const StringValue *>(this);
}

SymbolValue *Value::as_symbol() {
    assert(is_symbol());
    return static_cast<SymbolValue *>(this);
}

TrueValue *Value::as_true() {
    assert(is_true());
    return static_cast<TrueValue *>(this);
}

VoidPValue *Value::as_void_p() {
    assert(is_void_p());
    return static_cast<VoidPValue *>(this);
}

KernelModule *Value::as_kernel_module_for_method_binding() {
    return static_cast<KernelModule *>(this);
}

EnvValue *Value::as_env_value_for_method_binding() {
    return static_cast<EnvValue *>(this);
}

ParserValue *Value::as_parser_value_for_method_binding() {
    return static_cast<ParserValue *>(this);
}

SexpValue *Value::as_sexp_value_for_method_binding() {
    return static_cast<SexpValue *>(this);
}

const String *Value::identifier_str(Env *env, Conversion conversion) {
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

SymbolValue *Value::to_symbol(Env *env, Conversion conversion) {
    if (is_symbol()) {
        return as_symbol();
    } else if (is_string()) {
        return as_string()->to_symbol(env);
    } else if (conversion == Conversion::NullAllowed) {
        return nullptr;
    } else {
        env->raise("TypeError", "{} is not a symbol nor a string", inspect_str(env));
    }
}

void Value::set_singleton_class(ClassValue *klass) {
    klass->set_is_singleton(true);
    m_singleton_class = klass;
}

ClassValue *Value::singleton_class(Env *env) {
    if (m_singleton_class) {
        return m_singleton_class;
    }

    if (is_integer() || is_float() || is_symbol()) {
        env->raise("TypeError", "can't define singleton");
    }

    m_singleton_class = m_klass->subclass(env);
    m_singleton_class->set_is_singleton(true);
    if (is_module()) {
        auto name = String::format("#<Class:{}>", as_module()->class_name_or_blank());
        m_singleton_class->set_class_name(name);
    }
    return m_singleton_class;
}

ValuePtr Value::const_find(Env *env, SymbolValue *name, ConstLookupSearchMode search_mode, ConstLookupFailureMode failure_mode) {
    return m_klass->const_find(env, name, search_mode, failure_mode);
}

ValuePtr Value::const_get(SymbolValue *name) {
    return m_klass->const_get(name);
}

ValuePtr Value::const_fetch(SymbolValue *name) {
    return m_klass->const_fetch(name);
}

ValuePtr Value::const_set(SymbolValue *name, ValuePtr val) {
    return m_klass->const_set(name, val);
}

ValuePtr Value::ivar_get(Env *env, SymbolValue *name) {
    if (!name->is_ivar_name())
        env->raise("NameError", "`{}' is not allowed as an instance variable name", name->c_str());

    auto val = m_ivars.get(name, env);
    if (val)
        return val;
    else
        return NilValue::the();
}

ValuePtr Value::ivar_set(Env *env, SymbolValue *name, ValuePtr val) {
    if (!name->is_ivar_name())
        env->raise("NameError", "`{}' is not allowed as an instance variable name", name->c_str());

    m_ivars.put(name, val.value(), env);
    return val;
}

ValuePtr Value::instance_variables(Env *env) {
    ArrayValue *ary = new ArrayValue {};
    if (m_type == Value::Type::Integer || m_type == Value::Type::Float) {
        return ary;
    }
    for (auto pair : m_ivars) {
        ary->push(pair.first);
    }
    return ary;
}

ValuePtr Value::cvar_get(Env *env, SymbolValue *name) {
    ValuePtr val = cvar_get_or_null(env, name);
    if (val) {
        return val;
    } else {
        ModuleValue *module;
        if (is_module()) {
            module = as_module();
        } else {
            module = m_klass;
        }
        env->raise("NameError", "uninitialized class variable {} in {}", name->c_str(), module->class_name_or_blank());
    }
}

ValuePtr Value::cvar_get_or_null(Env *env, SymbolValue *name) {
    return m_klass->cvar_get_or_null(env, name);
}

ValuePtr Value::cvar_set(Env *env, SymbolValue *name, ValuePtr val) {
    return m_klass->cvar_set(env, name, val);
}

void Value::alias(Env *env, SymbolValue *new_name, SymbolValue *old_name) {
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

SymbolValue *Value::define_singleton_method(Env *env, SymbolValue *name, MethodFnPtr fn, int arity) {
    ClassValue *klass = singleton_class(env);
    klass->define_method(env, name, fn, arity);
    return name;
}

SymbolValue *Value::define_singleton_method(Env *env, SymbolValue *name, Block *block) {
    ClassValue *klass = singleton_class(env);
    klass->define_method(env, name, block);
    return name;
}

SymbolValue *Value::undefine_singleton_method(Env *env, SymbolValue *name) {
    return define_singleton_method(env, name, nullptr, 0);
}

SymbolValue *Value::define_method(Env *env, SymbolValue *name, MethodFnPtr fn, int arity) {
    if (!is_main_object()) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    m_klass->define_method(env, name, fn, arity);
    return name;
}

SymbolValue *Value::define_method(Env *env, SymbolValue *name, Block *block) {
    if (!is_main_object()) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    m_klass->define_method(env, name, block);
    return name;
}

SymbolValue *Value::undefine_method(Env *env, SymbolValue *name) {
    if (!is_main_object()) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    m_klass->undefine_method(env, name);
    return name;
}

ValuePtr Value::private_method(Env *env, ValuePtr name) {
    if (!is_main_object()) {
        printf("tried to call private_method on something that has no methods\n");
        abort();
    }
    return m_klass->private_method(env, name);
}

ValuePtr Value::protected_method(Env *env, ValuePtr name) {
    if (!is_main_object()) {
        printf("tried to call protected_method on something that has no methods\n");
        abort();
    }
    return m_klass->protected_method(env, name);
}

ValuePtr Value::public_send(Env *env, SymbolValue *name, size_t argc, ValuePtr *args, Block *block) {
    Method *method = find_method(env, name, MethodVisibility::Public);
    return method->call(env, this, argc, args, block);
}

ValuePtr Value::send(Env *env, SymbolValue *name, size_t argc, ValuePtr *args, Block *block) {
    Method *method = find_method(env, name, MethodVisibility::Private);
    return method->call(env, this, argc, args, block);
}

ValuePtr Value::send(Env *env, size_t argc, ValuePtr *args, Block *block) {
    auto name = args[0]->to_symbol(env, Value::Conversion::Strict);
    return send(env->caller(), name, argc - 1, args + 1, block);
}

Method *Value::find_method(Env *env, SymbolValue *method_name, MethodVisibility visibility_at_least, ModuleValue **matching_class_or_module, Method *after_method) {
    auto singleton = singleton_class();
    if (singleton) {
        Method *method = singleton_class()->find_method(env, method_name, matching_class_or_module, after_method);
        if (method) {
            if (method->is_undefined())
                env->raise("NoMethodError", "undefined method `{}' for {}:Class", method_name->c_str(), m_klass->class_name_or_blank());
            return method;
        }
    }
    ModuleValue *klass = this->klass();
    Method *method = klass->find_method(env, method_name, matching_class_or_module);
    if (method && !method->is_undefined()) {
        if (method->visibility() >= visibility_at_least) {
            return method;
        } else {
            env->raise("NoMethodError", "private method `{}' called for {}", method_name->c_str(), inspect_str(env));
        }
    } else if (method_name == SymbolValue::intern("inspect")) {
        env->raise("NoMethodError", "undefined method `inspect' for #<{}:{}>", klass->class_name_or_blank(), int_to_hex_string(object_id(), false));
    } else if (is_module()) {
        env->raise("NoMethodError", "undefined method `{}' for {}:{}", method_name->c_str(), klass->as_module()->class_name_or_blank(), klass->inspect_str(env));
    } else {
        env->raise("NoMethodError", "undefined method `{}' for {}", method_name->c_str(), inspect_str(env));
    }
}

ValuePtr Value::dup(Env *env) {
    switch (m_type) {
    case Value::Type::Array:
        return new ArrayValue { *as_array() };
    case Value::Type::Exception:
        return new ExceptionValue { *as_exception() };
    case Value::Type::Hash:
        return new HashValue { env, *as_hash() };
    case Value::Type::String:
        return new StringValue { *as_string() };
    case Value::Type::False:
    case Value::Type::Float:
    case Value::Type::Integer:
    case Value::Type::Nil:
    case Value::Type::Symbol:
    case Value::Type::True:
        return this;
    case Value::Type::Object:
        return new Value { *this };
    default:
        fprintf(stderr, "I don't know how to dup this kind of object yet %s (type = %d).\n", m_klass->class_name_or_blank()->c_str(), static_cast<int>(m_type));
        abort();
    }
}

bool Value::is_a(Env *env, ValuePtr val) {
    if (!val->is_module()) return false;
    ModuleValue *module = val->as_module();
    if (this == module) {
        return true;
    } else {
        ArrayValue *ancestors = m_klass->ancestors(env);
        for (ValuePtr m : *ancestors) {
            if (module == m->as_module()) {
                return true;
            }
        }
        return false;
    }
}

bool Value::respond_to(Env *env, ValuePtr name_val) {
    if (respond_to_method(env, SymbolValue::intern("respond_to?")))
        return send(env, SymbolValue::intern("respond_to?"), { name_val })->is_true();

    // Needed for BaseObject as it does not have an actual respond_to? method
    return respond_to_method(env, name_val);
}

bool Value::respond_to_method(Env *env, ValuePtr name_val) {
    Method *method;
    auto name_symbol = name_val->to_symbol(env, Conversion::Strict);
    if (singleton_class() && (method = singleton_class()->find_method(env, name_symbol))) {
        return !method->is_undefined();
    } else if ((method = m_klass->find_method(env, name_symbol))) {
        return !method->is_undefined();
    }
    return false;
}

const char *Value::defined(Env *env, SymbolValue *name, bool strict) {
    ValuePtr obj = nullptr;
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
        if (obj != NilValue::the()) return "global-variable";
    } else if (name->is_ivar_name()) {
        obj = ivar_get(env, name);
        if (obj != NilValue::the()) return "instance-variable";
    } else if (respond_to(env, name)) {
        return "method";
    }
    return nullptr;
}

ValuePtr Value::defined_obj(Env *env, SymbolValue *name, bool strict) {
    const char *result = defined(env, name, strict);
    if (result) {
        return new StringValue { result };
    } else {
        return NilValue::the();
    }
}

ProcValue *Value::to_proc(Env *env) {
    auto to_proc_symbol = SymbolValue::intern("to_proc");
    if (respond_to(env, to_proc_symbol)) {
        return send(env, to_proc_symbol)->as_proc();
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Proc)", m_klass->class_name_or_blank());
    }
}

// FIXME: this should actually live in the Kernel module which gets mixed into Object
ValuePtr Value::cmp(Env *env, ValuePtr other) {
    if (send(env, SymbolValue::intern("=="), { other })->is_truthy()) {
        return ValuePtr::integer(0);
    } else {
        return NilValue::the();
    }
}

ValuePtr Value::instance_eval(Env *env, ValuePtr string, Block *block) {
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
    return NilValue::the();
}

void Value::assert_type(Env *env, Value::Type expected_type, const char *expected_class_name) {
    if ((type()) != expected_type) {
        if (type() == ValueType::Nil) {
            // For some reason the errors with nil are slightly different...
            char first_letter = std::tolower(expected_class_name[0]);
            char const beginning[] = { first_letter, '\0' };
            env->raise("TypeError", "no implicit conversion from nil to {}{}", beginning, expected_class_name + 1);
        } else {
            env->raise("TypeError", "no implicit conversion of {} into {}", klass()->class_name_or_blank(), expected_class_name);
        }
    }
}

void Value::assert_not_frozen(Env *env) {
    if (is_frozen()) {
        env->raise("FrozenError", "can't modify frozen {}: {}", klass()->class_name_or_blank(), inspect_str(env));
    }
}

bool Value::neq(Env *env, ValuePtr other) {
    return send(env, SymbolValue::intern("=="), { other })->is_falsey();
}

const String *Value::inspect_str(Env *env) {
    auto inspected = send(env, SymbolValue::intern("inspect"));
    if (!inspected->is_string())
        return new String(""); // TODO: what to do here?
    return inspected->as_string()->to_low_level_string();
}

ValuePtr Value::enum_for(Env *env, const char *method, size_t argc, ValuePtr *args) {
    ValuePtr args2[argc + 1];
    args2[0] = SymbolValue::intern(method);
    for (size_t i = 0; i < argc; i++) {
        args2[i + 1] = args[i];
    }
    return this->public_send(env, SymbolValue::intern("enum_for"), argc + 1, args2);
}

void Value::visit_children(Visitor &visitor) {
    visitor.visit(m_singleton_class);
    visitor.visit(m_owner);
    for (auto pair : m_ivars) {
        visitor.visit(pair.first);
        visitor.visit(pair.second);
    }
}

void Value::gc_inspect(char *buf, size_t len) const {
    snprintf(buf, len, "<Value %p type=%d class=%p>", this, (int)m_type, m_klass);
}

}
