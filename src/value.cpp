#include "natalie.hpp"
#include "natalie/forward.hpp"

namespace Natalie {

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
        return as_symbol()->symbol;
    } else if (is_string()) {
        return as_string()->str;
    } else if (conversion == Conversion::NullAllowed) {
        return nullptr;
    } else {
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, this, "inspect", 0, NULL, NULL));
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
        NAT_RAISE(env, "TypeError", "%s is not a symbol nor a string", send(env, this, "inspect", 0, NULL, NULL));
    }
}

Value *const_get(Env *env, Value *parent, const char *name, bool strict) {
    Value *val = const_get_or_null(env, parent, name, strict, false);
    if (val) {
        return val;
    } else if (strict) {
        NAT_RAISE(env, "NameError", "uninitialized constant %S::%s", send(env, parent, "inspect", 0, NULL, NULL), name);
    } else {
        NAT_RAISE(env, "NameError", "uninitialized constant %s", name);
    }
}

Value *const_get_or_null(Env *env, Value *parent, const char *name, bool strict, bool define) {
    if (!parent->is_module()) {
        parent = parent->klass;
        assert(parent);
    }

    ModuleValue *search_parent;
    Value *val;

    if (!strict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = parent->as_module();
        while (!(val = static_cast<Value *>(hashmap_get(&search_parent->constants, name))) && search_parent->owner && search_parent->owner != NAT_OBJECT) {
            search_parent = search_parent->owner;
        }
        if (val) return val;
    }

    if (define) {
        // don't search superclasses
        val = static_cast<Value *>(hashmap_get(&parent->constants, name));
        if (val) return val;
    } else {
        // search in superclass hierarchy
        search_parent = parent->as_module();
        while (!(val = static_cast<Value *>(hashmap_get(&search_parent->constants, name))) && search_parent->superclass) {
            search_parent = search_parent->superclass;
        }
        if (val) return val;
    }

    if (!strict) {
        // lastly, search on the global, i.e. Object namespace
        val = static_cast<Value *>(hashmap_get(&NAT_OBJECT->constants, name));
        if (val) return val;
    }

    return NULL;
}

Value *const_set(Env *env, Value *parent, const char *name, Value *val) {
    if (!NAT_IS_MODULE_OR_CLASS(parent)) {
        parent = NAT_OBJ_CLASS(parent);
        assert(parent);
    }
    hashmap_remove(&parent->constants, name);
    hashmap_put(&parent->constants, name, val);
    if (val->is_module() && !val->owner) {
        val->owner = parent->as_module();
    }
    return val;
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

}
