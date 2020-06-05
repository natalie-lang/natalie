#include "natalie.hpp"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

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

ModuleValue::ModuleValue(Env *env)
    : Value { env, Value::Type::Module, const_get(env, NAT_OBJECT, "Module", true)->as_class() } { }

SymbolValue *StringValue::to_symbol(Env *env) {
    return symbol(env, str);
}

ArrayValue::ArrayValue(Env *env)
    : Value { env, Value::Type::Array, const_get(env, NAT_OBJECT, "Array", true)->as_class() } {
    vector_init(&ary, 0);
}

bool is_constant_name(const char *name) {
    return strlen(name) > 0 && isupper(name[0]);
}

bool is_global_name(const char *name) {
    return strlen(name) > 0 && name[0] == '$';
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

Value *var_get(Env *env, const char *key, ssize_t index) {
    if (index >= vector_size(env->vars)) {
        printf("Trying to get variable `%s' at index %zu which is not set.\n", key, index);
        abort();
    }
    Value *val = static_cast<Value *>(vector_get(env->vars, index));
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

Value *var_set(Env *env, const char *name, ssize_t index, bool allocate, Value *val) {
    size_t needed = index + 1;
    size_t current_size = vector_capacity(env->vars);
    if (needed > current_size) {
        if (allocate) {
            if (!env->vars) {
                env->vars = vector(needed);
                vector_set(env->vars, index, val);
            } else {
                vector_push(env->vars, val);
            }
        } else {
            printf("Tried to set a variable without first allocating space for it.\n");
            abort();
        }
    } else {
        vector_set(env->vars, index, val);
    }
    return val;
}

ExceptionValue *exception_new(Env *env, ClassValue *klass, const char *message) {
    ExceptionValue *obj = new ExceptionValue { env, klass };
    assert(message);
    Value *message_obj = string(env, message);
    initialize(env, obj, 1, &message_obj, NULL);
    return obj;
}

const char *find_current_method_name(Env *env) {
    while ((!env->method_name || strcmp(env->method_name, "<block>") == 0) && env->outer) {
        env = env->outer;
    }
    if (strcmp(env->method_name, "<main>") == 0) return NULL;
    return env->method_name;
}

// note: returns a heap pointer that the caller must free
static char *build_code_location_name(Env *env, Env *location_env) {
    do {
        if (location_env->method_name) {
            if (strcmp(location_env->method_name, "<block>") == 0) {
                if (location_env->outer) {
                    char *outer_name = build_code_location_name(env, location_env->outer);
                    char *name = heap_string(sprintf(env, "block in %s", outer_name)->str);
                    free(outer_name);
                    return name;
                } else {
                    return heap_string("block");
                }
            } else {
                return heap_string(location_env->method_name);
            }
        }
        location_env = location_env->outer;
    } while (location_env);
    return heap_string("(unknown)");
}

Value *raise(Env *env, ClassValue *klass, const char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    StringValue *message = vsprintf(env, message_format, args);
    va_end(args);
    ExceptionValue *exception = exception_new(env, klass, heap_string(message->str));
    raise_exception(env, exception);
    return exception;
}

Value *raise_exception(Env *env, Value *exception) {
    return raise_exception(env, exception->as_exception());
}

Value *raise_exception(Env *env, ExceptionValue *exception) {
    if (!exception->backtrace) { // only build a backtrace the first time the exception is raised (not on a re-raise)
        Value *bt = exception->backtrace = array_new(env);
        Env *bt_env = env;
        do {
            if (bt_env->file) {
                char *method_name = build_code_location_name(env, bt_env);
                array_push(env, bt, sprintf(env, "%s:%d:in `%s'", bt_env->file, bt_env->line, method_name));
                free(method_name);
            }
            bt_env = bt_env->caller;
        } while (bt_env);
    }
    while (env && !env->rescue) {
        env = env->caller;
    }
    assert(env);
    env->exception = exception;
    longjmp(env->jump_buf, 1);
}

Value *raise_local_jump_error(Env *env, Value *exit_value, const char *message) {
    ExceptionValue *exception = exception_new(env, const_get(env, NAT_OBJECT, "LocalJumpError", true)->as_class(), message);
    ivar_set(env, exception, "@exit_value", exit_value);
    raise_exception(env, exception);
    return exception;
}

Value *ivar_get(Env *env, Value *obj, const char *name) {
    assert(strlen(name) > 0);
    if (name[0] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an instance variable name", name);
    }
    if (obj->ivars.table == NULL) {
        hashmap_init(&obj->ivars, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(&obj->ivars, hashmap_alloc_key_string, free);
    }
    Value *val = static_cast<Value *>(hashmap_get(&obj->ivars, name));
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

Value *ivar_set(Env *env, Value *obj, const char *name, Value *val) {
    assert(strlen(name) > 0);
    if (name[0] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an instance variable name", name);
    }
    if (obj->ivars.table == NULL) {
        hashmap_init(&obj->ivars, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(&obj->ivars, hashmap_alloc_key_string, free);
    }
    hashmap_remove(&obj->ivars, name);
    hashmap_put(&obj->ivars, name, val);
    return val;
}

Value *cvar_get(Env *env, Value *obj, const char *name) {
    Value *val = cvar_get_or_null(env, obj, name);
    if (val) {
        return val;
    } else {
        ModuleValue *module;
        if (obj->is_module()) {
            module = obj->as_module();
        } else {
            module = obj->klass;
        }
        NAT_RAISE(env, "NameError", "uninitialized class variable %s in %s", name, module->class_name);
    }
}

Value *cvar_get_or_null(Env *env, Value *obj, const char *name) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    ModuleValue *module;
    if (obj->is_module()) {
        module = obj->as_module();
    } else {
        module = obj->klass;
    }

    Value *val = NULL;
    while (1) {
        if (module->cvars.table) {
            val = static_cast<Value *>(hashmap_get(&module->cvars, name));
            if (val) {
                return val;
            }
        }
        if (!module->superclass) {
            return NULL;
        }
        module = module->superclass;
    }
}

Value *cvar_set(Env *env, Value *obj, const char *name, Value *val) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    ModuleValue *module;
    if (obj->is_module()) {
        module = obj->as_module();
    } else {
        module = obj->klass;
    }

    ModuleValue *current = module;

    Value *exists = NULL;
    while (1) {
        if (current->cvars.table) {
            exists = static_cast<Value *>(hashmap_get(&current->cvars, name));
            if (exists) {
                hashmap_remove(&current->cvars, name);
                hashmap_put(&current->cvars, name, val);
                return val;
            }
        }
        if (!current->superclass) {
            if (module->cvars.table == NULL) {
                hashmap_init(&module->cvars, hashmap_hash_string, hashmap_compare_string, 10);
                hashmap_set_key_alloc_funcs(&module->cvars, hashmap_alloc_key_string, free);
            }
            hashmap_remove(&module->cvars, name);
            hashmap_put(&module->cvars, name, val);
            return val;
        }
        current = current->superclass;
    }
}

Value *global_get(Env *env, const char *name) {
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a global variable name", name);
    }
    Value *val = static_cast<Value *>(hashmap_get(env->global_env->globals, name));
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

Value *global_set(Env *env, const char *name, Value *val) {
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an global variable name", name);
    }
    hashmap_remove(env->global_env->globals, name);
    hashmap_put(env->global_env->globals, name, val);
    return val;
}

bool truthy(Value *obj) {
    if (obj == NULL || NAT_TYPE(obj) == Value::Type::False || NAT_TYPE(obj) == Value::Type::Nil) {
        return false;
    } else {
        return true;
    }
}

char *heap_string(const char *str) {
    return strdup(str);
}

static void init_class_or_module_data(Env *env, ModuleValue *module) {
    hashmap_init(&module->methods, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&module->methods, hashmap_alloc_key_string, free);
    hashmap_init(&module->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&module->constants, hashmap_alloc_key_string, free);
    module->cvars.table = NULL;
}

ClassValue *subclass(Env *env, Value *superclass, const char *name) {
    return subclass(env, superclass->as_class(), name);
}

ClassValue *subclass(Env *env, ClassValue *superclass, const char *name) {
    assert(superclass);
    ClassValue *klass = new ClassValue { env, Value::Type::Class, superclass->klass };
    if (superclass->singleton_class) {
        // TODO: what happens if the superclass gets a singleton_class later?
        klass->singleton_class = subclass(env, superclass->singleton_class, NULL);
    }
    klass->class_name = name ? heap_string(name) : NULL;
    klass->superclass = superclass;
    klass->env = new Env { &superclass->env };
    klass->env.outer = NULL;
    init_class_or_module_data(env, klass);
    return klass;
}

ModuleValue *module(Env *env, const char *name) {
    ModuleValue *module = new ModuleValue { env };
    module->class_name = name ? heap_string(name) : NULL;
    module->env = new Env { env };
    module->env.outer = NULL;
    init_class_or_module_data(env, module);
    return module;
}

void class_include(Env *env, ModuleValue *klass, ModuleValue *module) {
    klass->included_modules_count++;
    if (klass->included_modules_count == 1) {
        klass->included_modules_count++;
        klass->included_modules = static_cast<ModuleValue **>(calloc(2, sizeof(Value *)));
        klass->included_modules[0] = klass;
    } else {
        klass->included_modules = static_cast<ModuleValue **>(realloc(klass->included_modules, sizeof(Value *) * klass->included_modules_count));
    }
    klass->included_modules[klass->included_modules_count - 1] = module;
}

void class_prepend(Env *env, ModuleValue *klass, ModuleValue *module) {
    klass->included_modules_count++;
    if (klass->included_modules_count == 1) {
        klass->included_modules_count++;
        klass->included_modules = static_cast<ModuleValue **>(calloc(2, sizeof(Value *)));
        klass->included_modules[1] = klass;
    } else {
        klass->included_modules = static_cast<ModuleValue **>(realloc(klass->included_modules, sizeof(Value *) * klass->included_modules_count));
        for (ssize_t i = klass->included_modules_count - 1; i > 0; i--) {
            klass->included_modules[i] = klass->included_modules[i - 1];
        }
    }
    klass->included_modules[0] = module;
}

Value *initialize(Env *env, Value *obj, ssize_t argc, Value **args, Block *block) {
    ClassValue *klass = NAT_OBJ_CLASS(obj);
    ModuleValue *matching_class_or_module;
    Method *method = find_method(klass, "initialize", &matching_class_or_module);
    if (method) {
        call_method_on_class(env, klass, klass, "initialize", obj, argc, args, block);
    }
    return obj;
}

ClassValue *singleton_class(Env *env, Value *obj) {
    if (!obj->singleton_class) {
        obj->singleton_class = subclass(env, NAT_OBJ_CLASS(obj), NULL);
    }
    return obj->singleton_class;
}

IntegerValue *integer(Env *env, int64_t integer) {
    return new IntegerValue { env, integer };
}

SymbolValue *symbol(Env *env, const char *name) {
    SymbolValue *symbol = static_cast<SymbolValue *>(hashmap_get(env->global_env->symbols, name));
    if (symbol) {
        return symbol;
    } else {
        symbol = new SymbolValue { env, name };
        hashmap_put(env->global_env->symbols, name, symbol);
        return symbol;
    }
}

Vector *vector(ssize_t capacity) {
    Vector *vec = static_cast<Vector *>(malloc(sizeof(Vector)));
    vector_init(vec, capacity);
    return vec;
}

void vector_init(Vector *vec, ssize_t capacity) {
    vec->size = 0;
    vec->capacity = capacity;
    if (capacity > 0) {
        vec->data = static_cast<void **>(calloc(capacity, sizeof(void *)));
    } else {
        vec->data = NULL;
    }
}

ssize_t vector_size(Vector *vec) {
    if (!vec) return 0;
    return vec->size;
}

ssize_t vector_capacity(Vector *vec) {
    if (!vec) return 0;
    return vec->capacity;
}

void **vector_data(Vector *vec) {
    return vec->data;
}

void *vector_get(Vector *vec, ssize_t index) {
    return vec->data[index];
}

void vector_set(Vector *vec, ssize_t index, void *item) {
    assert(index < vec->capacity);
    vec->size = NAT_MAX(vec->size, index + 1);
    vec->data[index] = item;
}

void vector_free(Vector *vec) {
    free(vec->data);
    free(vec);
}

void vector_copy(Vector *dest, Vector *source) {
    vector_init(dest, source->capacity);
    dest->size = source->size;
    memcpy(dest->data, source->data, source->size * sizeof(void *));
}

static void vector_grow(Vector *vec, ssize_t capacity) {
    vec->data = static_cast<void **>(realloc(vec->data, sizeof(void *) * capacity));
    vec->capacity = capacity;
}

static void vector_grow_at_least(Vector *vec, ssize_t min_capacity) {
    ssize_t capacity = vec->capacity;
    if (capacity >= min_capacity) {
        return;
    }
    if (capacity > 0 && min_capacity <= capacity * NAT_VECTOR_GROW_FACTOR) {
        vector_grow(vec, capacity * NAT_VECTOR_GROW_FACTOR);
    } else {
        vector_grow(vec, min_capacity);
    }
}

void vector_push(Vector *vec, void *item) {
    ssize_t len = vec->size;
    if (vec->size >= vec->capacity) {
        vector_grow_at_least(vec, len + 1);
    }
    vec->size++;
    vec->data[len] = item;
}

void vector_add(Vector *new_vec, Vector *vec1, Vector *vec2) {
    vector_grow_at_least(new_vec, vec1->size + vec2->size);
    memcpy(new_vec->data, vec1->data, vec1->size * sizeof(void *));
    memcpy(new_vec->data + vec1->size, vec2->data, vec2->size * sizeof(void *));
    new_vec->size = vec1->size + vec2->size;
}

ArrayValue *array_new(Env *env) {
    ArrayValue *obj = new ArrayValue { env };
    return obj;
}

ArrayValue *array_with_vals(Env *env, ssize_t count, ...) {
    va_list args;
    va_start(args, count);
    ArrayValue *ary = array_new(env);
    for (ssize_t i = 0; i < count; i++) {
        array_push(env, ary, va_arg(args, Value *));
    }
    va_end(args);
    return ary;
}

ArrayValue *array_copy(Env *env, ArrayValue *source) {
    ArrayValue *obj = new ArrayValue { env };
    vector_copy(&obj->ary, &source->ary);
    return obj;
}

void array_push(Env *env, Value *array, Value *obj) {
    array_push(env, array->as_array(), obj);
}

void array_push(Env *env, ArrayValue *array, Value *obj) {
    assert(obj);
    vector_push(&array->ary, obj);
}

void array_push_splat(Env *env, Value *array, Value *obj) {
    array_push_splat(env, array->as_array(), obj);
}

void array_push_splat(Env *env, ArrayValue *array, Value *obj) {
    if (!obj->is_array() && respond_to(env, obj, "to_a")) {
        obj = send(env, obj, "to_a", 0, NULL, NULL);
    }
    if (obj->is_array()) {
        for (ssize_t i = 0; i < vector_size(&obj->as_array()->ary); i++) {
            vector_push(&array->ary, vector_get(&obj->as_array()->ary, i));
        }
    } else {
        array_push(env, array, obj);
    }
}

void array_expand_with_nil(Env *env, Value *array, ssize_t size) {
    array_expand_with_nil(env, array->as_array(), size);
}

void array_expand_with_nil(Env *env, ArrayValue *array, ssize_t size) {
    for (ssize_t i = vector_size(&array->ary); i < size; i++) {
        array_push(env, array, NAT_NIL);
    }
}

Value *splat(Env *env, Value *obj) {
    if (obj->is_array()) {
        return array_copy(env, obj->as_array());
    } else {
        return to_ary(env, obj, false);
    }
}

// this is used by the hashmap library and assumes that obj->env has been set
size_t hashmap_hash(const void *key) {
    HashKey *key_p = (HashKey *)key;
    assert(NAT_OBJ_HAS_ENV2(key_p));
    assert(key_p->env.caller);
    Value *hash_obj = send(&key_p->env, key_p->key, "hash", 0, NULL, NULL);
    assert(NAT_TYPE(hash_obj) == Value::Type::Integer);
    return NAT_INT_VALUE(hash_obj);
}

// this is used by the hashmap library to compare keys
int hashmap_compare(const void *a, const void *b) {
    HashKey *a_p = (HashKey *)a;
    HashKey *b_p = (HashKey *)b;
    // NOTE: Only one of the keys will have a relevant Env, i.e. the one with a non-null caller.
    // This is a bit of a hack to get around the fact that we can't pass any extra args to hashmap_* functions.
    // TODO: Write our own hashmap implementation that passes Env around. :^)
    Env *env = a_p->env.caller ? &a_p->env : &b_p->env;
    assert(env);
    assert(env->caller);
    Value *a_hash = send(env, a_p->key, "hash", 0, NULL, NULL);
    Value *b_hash = send(env, b_p->key, "hash", 0, NULL, NULL);
    assert(NAT_TYPE(a_hash) == Value::Type::Integer);
    assert(NAT_TYPE(b_hash) == Value::Type::Integer);
    return NAT_INT_VALUE(a_hash) - NAT_INT_VALUE(b_hash);
}

HashKey *hash_key_list_append(Env *env, HashValue *hash, Value *key, Value *val) {
    if (hash->key_list) {
        HashKey *first = hash->key_list;
        HashKey *last = hash->key_list->prev;
        HashKey *new_last = static_cast<HashKey *>(malloc(sizeof(HashKey)));
        new_last->key = key;
        new_last->val = val;
        // <first> ... <last> <new_last> -|
        // ^______________________________|
        new_last->prev = last;
        new_last->next = first;
        new_last->env = Env::new_detatched_block_env(env);
        new_last->env.caller = env;
        new_last->removed = false;
        first->prev = new_last;
        last->next = new_last;
        return new_last;
    } else {
        HashKey *node = static_cast<HashKey *>(malloc(sizeof(HashKey)));
        node->key = key;
        node->val = val;
        node->prev = node;
        node->next = node;
        node->env = Env::new_detatched_block_env(env);
        node->env.caller = env;
        node->removed = false;
        hash->key_list = node;
        return node;
    }
}

void hash_key_list_remove_node(HashValue *hash, HashKey *node) {
    HashKey *prev = node->prev;
    HashKey *next = node->next;
    // <prev> <-> <node> <-> <next>
    if (node == next) {
        // <node> -|
        // ^_______|
        node->prev = NULL;
        node->next = NULL;
        node->removed = true;
        hash->key_list = NULL;
        return;
    } else if (hash->key_list == node) {
        // starting point is the node to be removed, so shift them forward by one
        hash->key_list = next;
    }
    // remove the node
    node->removed = true;
    prev->next = next;
    next->prev = prev;
}

HashIter *hash_iter(Env *env, HashValue *hash) {
    if (!hash->key_list) {
        return NULL;
    } else {
        hash->hash_is_iterating = true;
        return hash->key_list;
    }
}

HashIter *hash_iter_prev(Env *env, HashValue *hash, HashIter *iter) {
    if (iter->prev == NULL || iter == hash->key_list) {
        // finished
        hash->hash_is_iterating = false;
        return NULL;
    } else if (iter->prev->removed) {
        return hash_iter_prev(env, hash, iter->prev);
    } else {
        return iter->prev;
    }
}

HashIter *hash_iter_next(Env *env, HashValue *hash, HashIter *iter) {
    if (iter->next == NULL || (!iter->removed && iter->next == hash->key_list)) {
        // finished
        hash->hash_is_iterating = false;
        return NULL;
    } else if (iter->next->removed) {
        return hash_iter_next(env, hash, iter->next);
    } else {
        return iter->next;
    }
}

HashValue *hash_new(Env *env) {
    HashValue *obj = new HashValue { env };
    obj->key_list = NULL;
    obj->hash_default_value = NAT_NIL;
    obj->hash_default_block = NULL;
    hashmap_init(&obj->hashmap, hashmap_hash, hashmap_compare, 256);
    return obj;
}

Value *hash_get(Env *env, Value *hash, Value *key) {
    return hash_get(env, hash->as_hash(), key);
}

Value *hash_get(Env *env, HashValue *hash, Value *key) {
    HashKey key_container;
    key_container.key = key;
    key_container.env = *env;
    HashVal *container = static_cast<HashVal *>(hashmap_get(&hash->hashmap, &key_container));
    Value *val = container ? container->val : NULL;
    return val;
}

Value *hash_get_default(Env *env, HashValue *hash, Value *key) {
    if (hash->hash_default_block) {
        Value *args[2] = { hash, key };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, hash->hash_default_block, 2, args, NULL);
    } else {
        return hash->hash_default_value;
    }
}

void hash_put(Env *env, Value *hash, Value *key, Value *val) {
    hash_put(env, hash->as_hash(), key, val);
}

void hash_put(Env *env, HashValue *hash, Value *key, Value *val) {
    assert(NAT_TYPE(hash) == Value::Type::Hash);
    HashKey key_container;
    key_container.key = key;
    key_container.env = *env;
    HashVal *container = static_cast<HashVal *>(hashmap_get(&hash->hashmap, &key_container));
    if (container) {
        container->key->val = val;
        container->val = val;
    } else {
        if (hash->hash_is_iterating) {
            NAT_RAISE(env, "RuntimeError", "can't add a new key into hash during iteration");
        }
        container = static_cast<HashVal *>(malloc(sizeof(HashVal)));
        container->key = hash_key_list_append(env, hash, key, val);
        container->val = val;
        hashmap_put(&hash->hashmap, container->key, container);
        // NOTE: caller must be current and relevant at all times
        // See note on hashmap_compare for more details
        container->key->env.caller = NULL;
    }
}

Value *hash_delete(Env *env, HashValue *hash, Value *key) {
    assert(hash->type == Value::Type::Hash);
    HashKey key_container;
    key_container.key = key;
    key_container.env = *env;
    HashVal *container = static_cast<HashVal *>(hashmap_remove(&hash->hashmap, &key_container));
    if (container) {
        hash_key_list_remove_node(hash, container->key);
        Value *val = container->val;
        free(container);
        return val;
    } else {
        return NULL;
    }
}

RegexpValue *regexp_new(Env *env, const char *pattern) {
    regex_t *regexp;
    OnigErrorInfo einfo;
    UChar *pat = (UChar *)pattern;
    int result = onig_new(&regexp, pat, pat + strlen((char *)pat),
        ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT, &einfo);
    if (result != ONIG_NORMAL) {
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result, &einfo);
        NAT_RAISE(env, "SyntaxError", (char *)s);
    }
    RegexpValue *obj = new RegexpValue { env };
    obj->regexp = regexp;
    obj->regexp_str = heap_string(pattern);
    return obj;
}

MatchDataValue *matchdata_new(Env *env, OnigRegion *region, StringValue *str_obj) {
    MatchDataValue *obj = new MatchDataValue { env };
    obj->matchdata_region = region;
    obj->matchdata_str = heap_string(str_obj->str);
    return obj;
}

Value *last_match(Env *env) {
    if (env->match) {
        return env->match;
    } else {
        return NAT_NIL;
    }
}

void int_to_string(int64_t num, char *buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = 0;
    } else {
        snprintf(buf, NAT_INT_64_MAX_BUF_LEN, "%" PRId64, num);
    }
}

void int_to_hex_string(int64_t num, char *buf, bool capitalize) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = 0;
    } else {
        if (capitalize) {
            snprintf(buf, NAT_INT_64_MAX_BUF_LEN, "0X%" PRIX64, num);
        } else {
            snprintf(buf, NAT_INT_64_MAX_BUF_LEN, "0x%" PRIx64, num);
        }
    }
}

static Method *method_from_fn(Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block)) {
    Method *method = static_cast<Method *>(malloc(sizeof(Method)));
    method->fn = fn;
    method->env.global_env = NULL;
    method->undefined = fn ? false : true;
    return method;
}

static Method *method_from_block(Block *block) {
    Method *method = static_cast<Method *>(malloc(sizeof(Method)));
    method->fn = block->fn;
    method->env = block->env;
    method->env.caller = NULL;
    method->undefined = false;
    return method;
}

void define_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block)) {
    Method *method = method_from_fn(fn);
    if (is_main_object(obj)) {
        free(hashmap_remove(&obj->klass->methods, name));
        hashmap_put(&obj->klass->methods, name, method);
    } else {
        free(hashmap_remove(&obj->as_module()->methods, name));
        hashmap_put(&obj->as_module()->methods, name, method);
    }
}

void define_method_with_block(Env *env, Value *obj, const char *name, Block *block) {
    Method *method = method_from_block(block);
    if (is_main_object(obj)) {
        free(hashmap_remove(&obj->klass->methods, name));
        hashmap_put(&obj->klass->methods, name, method);
    } else {
        free(hashmap_remove(&obj->as_module()->methods, name));
        hashmap_put(&obj->as_module()->methods, name, method);
    }
}

void define_singleton_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block)) {
    Method *method = method_from_fn(fn);
    ClassValue *klass = singleton_class(env, obj);
    free(hashmap_remove(&klass->methods, name));
    hashmap_put(&klass->methods, name, method);
}

void define_singleton_method_with_block(Env *env, Value *obj, const char *name, Block *block) {
    Method *method = method_from_block(block);
    ClassValue *klass = singleton_class(env, obj);
    free(hashmap_remove(&klass->methods, name));
    hashmap_put(&klass->methods, name, method);
}

void undefine_method(Env *env, Value *obj, const char *name) {
    define_method(env, obj, name, NULL);
}

void undefine_singleton_method(Env *env, Value *obj, const char *name) {
    define_singleton_method(env, obj, name, NULL);
}

ArrayValue *class_ancestors(Env *env, ModuleValue *klass) {
    ArrayValue *ancestors = array_new(env);
    do {
        if (klass->included_modules_count == 0) {
            // note: if there are included modules, then they will include this klass
            array_push(env, ancestors, klass);
        }
        for (ssize_t i = 0; i < klass->included_modules_count; i++) {
            array_push(env, ancestors, klass->included_modules[i]);
        }
        klass = klass->superclass;
    } while (klass);
    return ancestors;
}

bool is_a(Env *env, Value *obj, Value *klass_or_module) {
    return is_a(env, obj, klass_or_module->as_module());
}

bool is_a(Env *env, Value *obj, ModuleValue *klass_or_module) {
    if (obj == klass_or_module) {
        return true;
    } else {
        ArrayValue *ancestors = class_ancestors(env, NAT_OBJ_CLASS(obj));
        for (ssize_t i = 0; i < vector_size(&ancestors->ary); i++) {
            if (klass_or_module == vector_get(&ancestors->ary, i)) {
                return true;
            }
        }
        return false;
    }
}

const char *defined(Env *env, Value *receiver, const char *name) {
    Value *obj;
    if (is_constant_name(name)) {
        obj = const_get_or_null(env, receiver, name, false, false);
        if (obj) return "constant";
    } else if (is_global_name(name)) {
        obj = global_get(env, name);
        if (obj != NAT_NIL) return "global-variable";
    } else if (respond_to(env, receiver, name)) {
        return "method";
    }
    return NULL;
}

Value *defined_obj(Env *env, Value *receiver, const char *name) {
    const char *result = defined(env, receiver, name);
    if (result) {
        return string(env, result);
    } else {
        return NAT_NIL;
    }
}

Value *send(Env *env, Value *receiver, const char *sym, ssize_t argc, Value **args, Block *block) {
    assert(receiver);
    ClassValue *klass;
    if (NAT_TYPE(receiver) == Value::Type::Integer) {
        klass = NAT_INTEGER;
    } else {
        klass = receiver->singleton_class;
        if (klass) {
            ModuleValue *matching_class_or_module;
            Method *method = find_method(klass, sym, &matching_class_or_module);
            if (method) {
#ifdef NAT_DEBUG_METHOD_RESOLUTION
                if (strcmp(sym, "inspect") != 0) {
                    if (method->undefined) {
                        fprintf(stderr, "Method %s found on %s and is marked undefined\n", sym, matching_class_or_module->class_name);
                    } else if (matching_class_or_module == klass) {
                        fprintf(stderr, "Method %s found on the singleton klass of %s\n", sym, send(env, receiver, "inspect", 0, NULL, NULL)->str);
                    } else {
                        fprintf(stderr, "Method %s found on %s, which is an ancestor of the singleton klass of %s\n", sym, matching_class_or_module->class_name, send(env, receiver, "inspect", 0, NULL, NULL)->str);
                    }
                }
#endif
                if (method->undefined) {
                    NAT_RAISE(env, "NoMethodError", "undefined method `%s' for %s:Class", sym, receiver->as_class()->class_name);
                }
                return call_method_on_class(env, klass, NAT_OBJ_CLASS(receiver), sym, receiver, argc, args, block);
            }
        }
        klass = NAT_OBJ_CLASS(receiver);
    }
#ifdef NAT_DEBUG_METHOD_RESOLUTION
    if (strcmp(sym, "inspect") != 0) {
        fprintf(stderr, "Looking for method %s in the klass hierarchy of %s\n", sym, send(env, receiver, "inspect", 0, NULL, NULL)->str);
    }
#endif
    return call_method_on_class(env, klass, klass, sym, receiver, argc, args, block);
}

// supply an empty array and it will be populated with the method names as symbols
void methods(Env *env, ArrayValue *array, ModuleValue *klass) {
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(&klass->methods); iter; iter = hashmap_iter_next(&klass->methods, iter)) {
        const char *name = (char *)hashmap_iter_get_key(iter);
        array_push(env, array, symbol(env, name));
    }
    for (ssize_t i = 0; i < klass->included_modules_count; i++) {
        ModuleValue *module = klass->included_modules[i];
        for (iter = hashmap_iter(&module->methods); iter; iter = hashmap_iter_next(&module->methods, iter)) {
            const char *name = (char *)hashmap_iter_get_key(iter);
            array_push(env, array, symbol(env, name));
        }
    }
    if (klass->superclass) {
        return methods(env, array, klass->superclass);
    }
}

// returns the method and sets matching_class_or_module to where the method was found
Method *find_method(ModuleValue *klass, const char *method_name, ModuleValue **matching_class_or_module) {
    assert(NAT_TYPE(klass) == Value::Type::Class);

    Method *method;
    if (klass->included_modules_count == 0) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        method = static_cast<Method *>(hashmap_get(&klass->methods, method_name));
        if (method) {
            *matching_class_or_module = klass;
            return method;
        }
    }

    for (ssize_t i = 0; i < klass->included_modules_count; i++) {
        ModuleValue *module = klass->included_modules[i];
        method = static_cast<Method *>(hashmap_get(&module->methods, method_name));
        if (method) {
            *matching_class_or_module = module;
            return method;
        }
    }

    if (klass->superclass) {
        return find_method(klass->superclass, method_name, matching_class_or_module);
    } else {
        return NULL;
    }
}

Method *find_method_without_undefined(ClassValue *klass, const char *method_name, ModuleValue **matching_class_or_module) {
    Method *method = find_method(klass, method_name, matching_class_or_module);
    if (method && method->undefined) {
        return NULL;
    } else {
        return method;
    }
}

Value *call_method_on_class(Env *env, ClassValue *klass, Value *instance_class, const char *method_name, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(klass != NULL);
    assert(NAT_TYPE(klass) == Value::Type::Class);

    ModuleValue *matching_class_or_module;
    Method *method = find_method(klass, method_name, &matching_class_or_module);
    if (method && !method->undefined) {
#ifdef NAT_DEBUG_METHOD_RESOLUTION
        if (strcmp(method_name, "inspect") != 0) {
            fprintf(stderr, "Calling method %s from %s\n", method_name, matching_class_or_module->class_name);
        }
#endif
        Env *closure_env;
        if (NAT_OBJ_HAS_ENV(method)) {
            closure_env = &method->env;
        } else {
            closure_env = &matching_class_or_module->env;
        }
        Env e = Env::new_block_env(closure_env, env);
        e.file = env->file;
        e.line = env->line;
        e.method_name = method_name;
        e.block = block;
        return method->fn(&e, self, argc, args, block);
    } else {
        NAT_RAISE(env, "NoMethodError", "undefined method `%s' for %v", method_name, instance_class);
    }
}

Value *call_begin(Env *env, Value *self, Value *(*block_fn)(Env *, Value *)) {
    Env e = Env::new_block_env(env, env);
    return block_fn(&e, self);
}

bool respond_to(Env *env, Value *obj, const char *name) {
    ModuleValue *matching_class_or_module;
    if (NAT_TYPE(obj) == Value::Type::Integer) {
        ClassValue *klass = NAT_INTEGER;
        if (find_method_without_undefined(klass, name, &matching_class_or_module)) {
            return true;
        } else {
            return false;
        }
    } else if (obj->singleton_class && find_method_without_undefined(obj->singleton_class, name, &matching_class_or_module)) {
        return true;
    } else if (find_method_without_undefined(NAT_OBJ_CLASS(obj), name, &matching_class_or_module)) {
        return true;
    } else {
        return false;
    }
}

Block *block_new(Env *env, Value *self, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *)) {
    Block *block = static_cast<Block *>(malloc(sizeof(Block)));
    block->env = *env;
    block->env.caller = NULL;
    block->self = self;
    block->fn = fn;
    return block;
}

Value *_run_block_internal(Env *env, Block *the_block, ssize_t argc, Value **args, Block *block) {
    if (!the_block) {
        abort();
        NAT_RAISE(env, "LocalJumpError", "no block given");
    }
    Env e = Env::new_block_env(&the_block->env, env);
    return the_block->fn(&e, the_block->self, argc, args, block);
}

ProcValue *proc_new(Env *env, Block *block) {
    ProcValue *obj = new ProcValue { env };
    obj->block = block;
    return obj;
}

ProcValue *to_proc(Env *env, Value *obj) {
    if (obj->is_proc()) {
        return obj->as_proc();
    } else if (respond_to(env, obj, "to_proc")) {
        return send(env, obj, "to_proc", 0, NULL, NULL)->as_proc();
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Proc)", NAT_OBJ_CLASS(obj)->class_name);
    }
}

ProcValue *lambda(Env *env, Block *block) {
    ProcValue *lambda = proc_new(env, block);
    lambda->lambda = true;
    return lambda;
}

StringValue *string_n(Env *env, const char *str, ssize_t len) {
    StringValue *obj = new StringValue { env };
    char *copy = static_cast<char *>(malloc(len + 1));
    memcpy(copy, str, len);
    obj->str = copy;
    obj->str[len] = 0;
    obj->str_len = len;
    obj->str_cap = len;
    obj->encoding = Encoding::UTF_8; // TODO: inherit from encoding of file?
    return obj;
}

StringValue *string(Env *env, const char *str) {
    ssize_t len = strlen(str);
    return string_n(env, str, len);
}

void grow_string(Env *env, StringValue *obj, ssize_t capacity) {
    ssize_t len = strlen(obj->str);
    assert(capacity >= len);
    obj->str = static_cast<char *>(realloc(obj->str, capacity + 1));
    obj->str_cap = capacity;
}

void grow_string_at_least(Env *env, StringValue *obj, ssize_t min_capacity) {
    ssize_t capacity = obj->str_cap;
    if (capacity >= min_capacity)
        return;
    if (capacity > 0 && min_capacity <= capacity * NAT_STRING_GROW_FACTOR) {
        grow_string(env, obj, capacity * NAT_STRING_GROW_FACTOR);
    } else {
        grow_string(env, obj, min_capacity);
    }
}

void string_append(Env *env, Value *val, const char *s) {
    string_append(env, val->as_string(), s);
}

void string_append(Env *env, StringValue *str, const char *s) {
    ssize_t new_len = strlen(s);
    if (new_len == 0) return;
    ssize_t total_len = str->str_len + new_len;
    grow_string_at_least(env, str, total_len);
    strcat(str->str, s);
    str->str_len = total_len;
}

void string_append_char(Env *env, StringValue *str, char c) {
    ssize_t total_len = str->str_len + 1;
    grow_string_at_least(env, str, total_len);
    str->str[total_len - 1] = c;
    str->str[total_len] = 0;
    str->str_len = total_len;
}

void string_append_string(Env *env, Value *val, Value *val2) {
    string_append_string(env, val->as_string(), val2->as_string());
}

void string_append_string(Env *env, StringValue *str, StringValue *str2) {
    if (str2->str_len == 0) return;
    ssize_t total_len = str->str_len + str2->str_len;
    grow_string_at_least(env, str, total_len);
    strncat(str->str, str2->str, str2->str_len);
    str->str_len = total_len;
}

#define NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, message_format, ...)                      \
    {                                                                                                 \
        Value *Encoding = const_get(env, NAT_OBJECT, "Encoding", true);                               \
        Value *InvalidByteSequenceError = const_get(env, Encoding, "InvalidByteSequenceError", true); \
        raise(env, InvalidByteSequenceError, message_format, ##__VA_ARGS__);                          \
    }

ArrayValue *string_chars(Env *env, StringValue *str) {
    ArrayValue *ary = array_new(env);
    StringValue *c;
    char buffer[5];
    switch (str->encoding) {
    case Encoding::UTF_8:
        for (ssize_t i = 0; i < str->str_len; i++) {
            buffer[0] = str->str[i];
            if (((unsigned char)buffer[0] >> 3) == 30) { // 11110xxx, 4 bytes
                if (i + 3 >= str->str_len) abort(); //NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, send(env, str, "inspect", 0, NULL, NULL));
                buffer[1] = str->str[++i];
                buffer[2] = str->str[++i];
                buffer[3] = str->str[++i];
                buffer[4] = 0;
            } else if (((unsigned char)buffer[0] >> 4) == 14) { // 1110xxxx, 3 bytes
                if (i + 2 >= str->str_len) abort(); //NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, send(env, str, "inspect", 0, NULL, NULL));
                buffer[1] = str->str[++i];
                buffer[2] = str->str[++i];
                buffer[3] = 0;
            } else if (((unsigned char)buffer[0] >> 5) == 6) { // 110xxxxx, 2 bytes
                if (i + 1 >= str->str_len) abort(); //NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, send(env, str, "inspect", 0, NULL, NULL));
                buffer[1] = str->str[++i];
                buffer[2] = 0;
            } else {
                buffer[1] = 0;
            }
            c = string(env, buffer);
            c->encoding = Encoding::UTF_8;
            array_push(env, ary, c);
        }
        break;
    case Encoding::ASCII_8BIT:
        for (ssize_t i = 0; i < str->str_len; i++) {
            buffer[0] = str->str[i];
            buffer[1] = 0;
            c = string(env, buffer);
            c->encoding = Encoding::ASCII_8BIT;
            array_push(env, ary, c);
        }
        break;
    }
    return ary;
}

StringValue *sprintf(Env *env, const char *format, ...) {
    va_list args;
    va_start(args, format);
    StringValue *out = vsprintf(env, format, args);
    va_end(args);
    return out;
}

StringValue *vsprintf(Env *env, const char *format, va_list args) {
    StringValue *out = string(env, "");
    ssize_t len = strlen(format);
    StringValue *inspected;
    char buf[NAT_INT_64_MAX_BUF_LEN];
    for (ssize_t i = 0; i < len; i++) {
        char c = format[i];
        if (c == '%') {
            char c2 = format[++i];
            switch (c2) {
            case 's':
                string_append(env, out, va_arg(args, char *));
                break;
            case 'S':
                string_append_string(env, out, va_arg(args, StringValue *));
                break;
            case 'i':
            case 'd':
                int_to_string(va_arg(args, int64_t), buf);
                string_append(env, out, buf);
                break;
            case 'x':
                int_to_hex_string(va_arg(args, int64_t), buf, false);
                string_append(env, out, buf);
                break;
            case 'X':
                int_to_hex_string(va_arg(args, int64_t), buf, true);
                string_append(env, out, buf);
                break;
            case 'v':
                inspected = send(env, va_arg(args, Value *), "inspect", 0, NULL, NULL)->as_string();
                string_append(env, out, inspected->str);
                break;
            case '%':
                string_append_char(env, out, '%');
                break;
            default:
                fprintf(stderr, "Unknown format specifier: %%%c", c2);
                abort();
            }
        } else {
            string_append_char(env, out, c);
        }
    }
    return out;
}

RangeValue *range_new(Env *env, Value *begin, Value *end, bool exclude_end) {
    RangeValue *obj = new RangeValue { env };
    obj->range_begin = begin;
    obj->range_end = end;
    obj->range_exclude_end = exclude_end;
    return obj;
}

Value *dup(Env *env, Value *obj) {
    switch (NAT_TYPE(obj)) {
    case Value::Type::Array:
        return array_copy(env, obj->as_array());
    case Value::Type::String:
        return string(env, obj->as_string()->str);
    case Value::Type::Symbol:
        return symbol(env, obj->as_symbol()->symbol);
    case Value::Type::False:
    case Value::Type::Nil:
    case Value::Type::True:
        return obj;
    default:
        fprintf(stderr, "I don't know how to dup this kind of object yet %d.\n", static_cast<int>(obj->type));
        abort();
    }
}

Value *bool_not(Env *env, Value *val) {
    if (truthy(val)) {
        return NAT_FALSE;
    } else {
        return NAT_TRUE;
    }
}

void alias(Env *env, Value *self, const char *new_name, const char *old_name) {
    if (self->is_integer() || self->is_symbol()) {
        NAT_RAISE(env, "TypeError", "no klass to make alias");
    }
    if (is_main_object(self)) {
        self = NAT_OBJ_CLASS(self);
    }
    ModuleValue *klass;
    if (self->is_module()) {
        klass = self->as_module();
    } else {
        klass = singleton_class(env, self);
    }
    ModuleValue *matching_class_or_module;
    Method *method = find_method(klass->as_module(), old_name, &matching_class_or_module);
    if (!method) {
        NAT_RAISE(env, "NameError", "undefined method `%s' for `%v'", old_name, klass);
    }
    free(hashmap_remove(&klass->methods, new_name));
    Method *method_copy = static_cast<Method *>(malloc(sizeof(Method)));
    memcpy(method_copy, method, sizeof(Method));
    hashmap_put(&klass->methods, new_name, method_copy);
}

void run_at_exit_handlers(Env *env) {
    ArrayValue *at_exit_handlers = global_get(env, "$NAT_at_exit_handlers")->as_array();
    assert(at_exit_handlers);
    for (int i = vector_size(&at_exit_handlers->ary) - 1; i >= 0; i--) {
        Value *proc = static_cast<Value *>(vector_get(&at_exit_handlers->ary, i));
        assert(proc);
        assert(proc->is_proc());
        NAT_RUN_BLOCK_WITHOUT_BREAK(env, proc->as_proc()->block, 0, NULL, NULL);
    }
}

void print_exception_with_backtrace(Env *env, ExceptionValue *exception) {
    IoValue *stderr = global_get(env, "$stderr")->as_io();
    int fd = stderr->fileno;
    if (vector_size(&exception->backtrace->ary) > 0) {
        dprintf(fd, "Traceback (most recent call last):\n");
        for (int i = vector_size(&exception->backtrace->ary) - 1; i > 0; i--) {
            StringValue *line = static_cast<StringValue *>(vector_get(&exception->backtrace->ary, i));
            assert(NAT_TYPE(line) == Value::Type::String);
            dprintf(fd, "        %d: from %s\n", i, line->str);
        }
        StringValue *line = static_cast<StringValue *>(vector_get(&exception->backtrace->ary, 0));
        dprintf(fd, "%s: ", line->str);
    }
    dprintf(fd, "%s (%s)\n", exception->message, NAT_OBJ_CLASS(exception)->class_name);
}

void handle_top_level_exception(Env *env, bool run_exit_handlers) {
    ExceptionValue *exception = env->exception->as_exception();
    env->rescue = false;
    if (is_a(env, exception, const_get(env, NAT_OBJECT, "SystemExit", true)->as_class())) {
        Value *status_obj = ivar_get(env, exception, "@status");
        if (run_exit_handlers) run_at_exit_handlers(env);
        if (NAT_TYPE(status_obj) == Value::Type::Integer) {
            int64_t val = NAT_INT_VALUE(status_obj);
            if (val >= 0 && val <= 255) {
                exit(val);
            } else {
                exit(1);
            }
        } else {
            exit(1);
        }
    } else {
        print_exception_with_backtrace(env, exception);
    }
}

void object_pointer_id(Value *obj, char *buf) {
    snprintf(buf, NAT_OBJECT_POINTER_BUF_LENGTH, "%p", obj);
}

int64_t object_id(Env *env, Value *obj) {
    if (NAT_TYPE(obj) == Value::Type::Integer) {
        return (int64_t)obj;
    } else {
        return (int64_t)obj / 2;
    }
}

int quicksort_partition(Env *env, Value *ary[], int start, int end) {
    Value *pivot = ary[end];
    int pIndex = start;
    Value *temp;

    for (int i = start; i < end; i++) {
        Value *compare = send(env, ary[i], "<=>", 1, &pivot, NULL);
        if (NAT_INT_VALUE(compare) < 0) {
            temp = ary[i];
            ary[i] = ary[pIndex];
            ary[pIndex] = temp;
            pIndex++;
        }
    }
    temp = ary[end];
    ary[end] = ary[pIndex];
    ary[pIndex] = temp;
    return pIndex;
}

void quicksort(Env *env, Value *ary[], int start, int end) {
    if (start < end) {
        int pIndex = quicksort_partition(env, ary, start, end);
        quicksort(env, ary, start, pIndex - 1);
        quicksort(env, ary, pIndex + 1, end);
    }
}

ArrayValue *to_ary(Env *env, Value *obj, bool raise_for_non_array) {
    if (obj->is_array()) {
        return obj->as_array();
    } else if (respond_to(env, obj, "to_ary")) {
        Value *ary = send(env, obj, "to_ary", 0, NULL, NULL);
        if (ary->is_array()) {
            return ary->as_array();
        } else if (ary->is_nil() || !raise_for_non_array) {
            ary = array_new(env);
            array_push(env, ary, obj);
            return ary->as_array();
        } else {
            const char *class_name = NAT_OBJ_CLASS(obj)->class_name;
            NAT_RAISE(env, "TypeError", "can't convert %s to Array (%s#to_ary gives %s)", class_name, class_name, NAT_OBJ_CLASS(ary)->class_name);
        }
    } else {
        ArrayValue *ary = array_new(env);
        array_push(env, ary, obj);
        return ary;
    }
}

static Value *splat_value(Env *env, Value *value, ssize_t index, ssize_t offset_from_end) {
    Value *splat = array_new(env);
    if (value->is_array() && index < vector_size(&value->as_array()->ary) - offset_from_end) {
        for (ssize_t s = index; s < vector_size(&value->as_array()->ary) - offset_from_end; s++) {
            array_push(env, splat, static_cast<Value *>(vector_get(&value->as_array()->ary, s)));
        }
    }
    return splat;
}

Value *arg_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    bool has_default = default_value != NAT_NIL;
    bool defaults_on_left = !defaults_on_right;
    int required_count = total_count - default_count;
    Value *return_value = value;
    for (ssize_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);

        if (splat && i == path_size - 1) {
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {

                int64_t ary_len = (int64_t)vector_size(&return_value->as_array()->ary);

                int first_required = default_count;
                int remain = ary_len - required_count;

                if (has_default && index >= remain && index < first_required && defaults_on_left) {
                    // this is an arg with a default value;
                    // not enough values to fill all the required args and this one
                    return default_value;
                }

                if (i == 0 && path_size == 1) {
                    // shift index left if needed
                    int extra_count = ary_len - required_count;
                    if (defaults_on_left && extra_count > 0 && default_count >= extra_count && index >= extra_count) {
                        index -= (default_count - extra_count);
                    } else if (ary_len <= required_count && defaults_on_left) {
                        index -= (default_count);
                    }
                }

                if (index < 0) {
                    // negative offset index should go from the right
                    if (ary_len >= total_count) {
                        index = ary_len + index;
                    } else {
                        // not enough values to fill from the right
                        // also, assume there is a splat prior to this index
                        index = total_count - 1 + index;
                    }
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = static_cast<Value *>(vector_get(&return_value->as_array()->ary, index));

                } else {
                    // index past the end of the array, so use default
                    return_value = default_value;
                }

            } else if (index == 0) {
                // not an array, so nothing to do (the object itself is returned)
                // no-op

            } else {
                // not an array, and index isn't zero
                return_value = default_value;
            }
        }
    }
    va_end(args);
    return return_value;
}

Value *array_value_by_path(Env *env, Value *value, Value *default_value, bool splat, int offset_from_end, ssize_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    Value *return_value = value;
    for (ssize_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);
        if (splat && i == path_size - 1) {
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {

                int64_t ary_len = (int64_t)vector_size(&return_value->as_array()->ary);

                if (index < 0) {
                    // negative offset index should go from the right
                    index = ary_len + index;
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = static_cast<Value *>(vector_get(&return_value->as_array()->ary, index));

                } else {
                    // index past the end of the array, so use default
                    return_value = default_value;
                }

            } else if (index == 0) {
                // not an array, so nothing to do (the object itself is returned)
                // no-op

            } else {
                // not an array, and index isn't zero
                return_value = default_value;
            }
        }
    }
    va_end(args);
    return return_value;
}

Value *kwarg_value_by_name(Env *env, Value *args, const char *name, Value *default_value) {
    return kwarg_value_by_name(env, args->as_array(), name, default_value);
}

Value *kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, Value *default_value) {
    Value *hash;
    if (vector_size(&args->ary) == 0) {
        hash = hash_new(env);
    } else {
        hash = static_cast<Value *>(vector_get(&args->ary, vector_size(&args->ary) - 1));
        if (NAT_TYPE(hash) != Value::Type::Hash) {
            hash = hash_new(env);
        }
    }
    Value *value = hash_get(env, hash, symbol(env, name));
    if (!value) {
        if (default_value) {
            return default_value;
        } else {
            NAT_RAISE(env, "ArgumentError", "missing keyword: :%s", name);
        }
    }
    return value;
}

ArrayValue *args_to_array(Env *env, ssize_t argc, Value **args) {
    ArrayValue *ary = array_new(env);
    for (ssize_t i = 0; i < argc; i++) {
        array_push(env, ary, args[i]);
    }
    return ary;
}

// much like args_to_array above, but when a block is given a single arg,
// and the block wants multiple args, call to_ary on the first arg and return that
ArrayValue *block_args_to_array(Env *env, ssize_t signature_size, ssize_t argc, Value **args) {
    if (argc == 1 && signature_size > 1) {
        return to_ary(env, args[0], true);
    }
    return args_to_array(env, argc, args);
}

EncodingValue *encoding(Env *env, Encoding num, ArrayValue *names) {
    EncodingValue *encoding = new EncodingValue { env };
    encoding->encoding_num = num;
    encoding->encoding_names = names;
    freeze_object(names);
    return encoding;
}

Value *eval_class_or_module_body(Env *env, Value *class_or_module, Value *(*fn)(Env *, Value *)) {
    Env body_env = new Env { env };
    body_env.caller = env;
    Value *result = fn(&body_env, class_or_module);
    body_env.caller = NULL;
    return result;
}

void arg_spread(Env *env, ssize_t argc, Value **args, char *arrangement, ...) {
    va_list va_args;
    va_start(va_args, arrangement);
    ssize_t len = strlen(arrangement);
    ssize_t arg_index = 0;
    Value *obj;
    bool *bool_ptr;
    int *int_ptr;
    char **str_ptr;
    void **void_ptr;
    Value **obj_ptr;
    for (ssize_t i = 0; i < len; i++) {
        char c = arrangement[i];
        switch (c) {
        case 'o':
            obj_ptr = va_arg(va_args, Value **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            *obj_ptr = obj;
            break;
        case 'i':
            int_ptr = va_arg(va_args, int *);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            NAT_ASSERT_TYPE(obj, Value::Type::Integer, "Integer");
            *int_ptr = NAT_INT_VALUE(obj);
            break;
        case 's':
            str_ptr = va_arg(va_args, char **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            if (obj == NAT_NIL) {
                *str_ptr = NULL;
            } else {
                NAT_ASSERT_TYPE(obj, Value::Type::String, "String");
            }
            *str_ptr = obj->as_string()->str;
            break;
        case 'b':
            bool_ptr = va_arg(va_args, bool *);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            *bool_ptr = truthy(obj);
            break;
        case 'v':
            void_ptr = va_arg(va_args, void **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            obj = ivar_get(env, obj, "@_ptr");
            assert(obj->type == Value::Type::VoidP);
            *void_ptr = obj->as_void_p()->void_ptr;
            break;
        default:
            fprintf(stderr, "Unknown arg spread arrangement specifier: %%%c", c);
            abort();
        }
    }
    va_end(va_args);
}

VoidPValue *void_ptr(Env *env, void *ptr) {
    VoidPValue *obj = new VoidPValue { env };
    obj->void_ptr = ptr;
    return obj;
}

}
