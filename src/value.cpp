#include "natalie.hpp"
#include "natalie/forward.hpp"

namespace Natalie {

Value *Value::initialize(Env *env, ssize_t argc, Value **args, Block *block) {
    ClassValue *klass = this->klass;
    ModuleValue *matching_class_or_module;
    Method *method = klass->find_method("initialize", &matching_class_or_module);
    if (method) {
        klass->call_method(env, klass, "initialize", this, argc, args, block);
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

ModuleValue *Value::as_module() {
    assert(is_module() || is_class());
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

SymbolValue *Value::as_symbol() {
    assert(is_symbol());
    return static_cast<SymbolValue *>(this);
}

VoidPValue *Value::as_void_p() {
    assert(is_void_p());
    return static_cast<VoidPValue *>(this);
}

const char *Value::identifier_str(Env *env, Conversion conversion) {
    if (is_symbol()) {
        return as_symbol()->c_str();
    } else if (is_string()) {
        return as_string()->c_str();
    } else if (conversion == Conversion::NullAllowed) {
        return nullptr;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", this->send(env, "inspect", 0, nullptr, nullptr));
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
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", this->send(env, "inspect", 0, nullptr, nullptr));
    }
}

ClassValue *Value::singleton_class(Env *env) {
    if (m_singleton_class == nullptr) {
        m_singleton_class = klass->subclass(env, nullptr);
    }
    return m_singleton_class;
}

Value *Value::const_get(Env *env, const char *name, bool strict) {
    return this->klass->const_get(env, name, strict);
}

Value *Value::const_get_or_null(Env *env, const char *name, bool strict, bool define) {
    return this->klass->const_get_or_null(env, name, strict, define);
}

Value *Value::const_set(Env *env, const char *name, Value *val) {
    return this->klass->const_set(env, name, val);
}

Value *Value::ivar_get(Env *env, const char *name) {
    assert(strlen(name) > 0);
    if (name[0] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an instance variable name", name);
    }
    init_ivars();
    Value *val = static_cast<Value *>(hashmap_get(&this->ivars, name));
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

Value *Value::ivar_set(Env *env, const char *name, Value *val) {
    assert(strlen(name) > 0);
    if (name[0] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an instance variable name", name);
    }
    init_ivars();
    hashmap_remove(&this->ivars, name);
    hashmap_put(&this->ivars, name, val);
    return val;
}

void Value::init_ivars() {
    if (this->ivars.table) return;
    hashmap_init(&this->ivars, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&this->ivars, hashmap_alloc_key_string, free);
}

Value *Value::cvar_get(Env *env, const char *name) {
    Value *val = this->cvar_get_or_null(env, name);
    if (val) {
        return val;
    } else {
        ModuleValue *module;
        if (this->is_module()) {
            module = this->as_module();
        } else {
            module = this->klass;
        }
        NAT_RAISE(env, "NameError", "uninitialized class variable %s in %s", name, module->class_name());
    }
}

Value *Value::cvar_get_or_null(Env *env, const char *name) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    return this->klass->cvar_get_or_null(env, name);
}

Value *Value::cvar_set(Env *env, const char *name, Value *val) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    return this->klass->cvar_set(env, name, val);
}

void Value::alias(Env *env, const char *new_name, const char *old_name) {
    if (is_integer() || is_symbol()) {
        NAT_RAISE(env, "TypeError", "no klass to make alias");
    }
    if (is_main_object(this)) {
        this->klass->alias(env, new_name, old_name);
    } else if (this->is_module()) {
        this->as_module()->alias(env, new_name, old_name);
    } else {
        this->singleton_class(env)->alias(env, new_name, old_name);
    }
}

void Value::define_singleton_method(Env *env, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block)) {
    ClassValue *klass = singleton_class(env);
    klass->define_method(env, name, fn);
}

void Value::define_singleton_method_with_block(Env *env, const char *name, Block *block) {
    ClassValue *klass = singleton_class(env);
    klass->define_method_with_block(env, name, block);
}

void Value::undefine_singleton_method(Env *env, const char *name) {
    define_singleton_method(env, name, nullptr);
}

void Value::define_method(Env *env, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block)) {
    if (!is_main_object(this)) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    klass->define_method(env, name, fn);
}

void Value::define_method_with_block(Env *env, const char *name, Block *block) {
    if (!is_main_object(this)) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    klass->define_method_with_block(env, name, block);
}

void Value::undefine_method(Env *env, const char *name) {
    if (!is_main_object(this)) {
        printf("tried to call define_method on something that has no methods\n");
        abort();
    }
    klass->undefine_method(env, name);
}

Value *Value::send(Env *env, const char *sym, ssize_t argc, Value **args, Block *block) {
    ClassValue *klass;
    klass = singleton_class();
    if (klass) {
        ModuleValue *matching_class_or_module;
        Method *method = klass->find_method(sym, &matching_class_or_module);
        if (method) {
            if (method->undefined) {
                NAT_RAISE(env, "NoMethodError", "undefined method `%s' for %s:Class", sym, this->klass->class_name());
            }
            return klass->call_method(env, this->klass, sym, this, argc, args, block);
        }
    }
    klass = this->klass;
    return klass->call_method(env, klass, sym, this, argc, args, block);
}

Value *Value::dup(Env *env) {
    switch (type) {
    case Value::Type::Array:
        return new ArrayValue { *as_array() };
    case Value::Type::String:
        return new StringValue { *as_string() };
    case Value::Type::False:
    case Value::Type::Integer:
    case Value::Type::Nil:
    case Value::Type::Symbol:
    case Value::Type::True:
        return this;
    default:
        fprintf(stderr, "I don't know how to dup this kind of object yet %s (type = %d).\n", klass->class_name(), static_cast<int>(type));
        abort();
    }
}
}
