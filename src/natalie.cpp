#include "natalie.hpp"
#include "gc.hpp"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

namespace Natalie {

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
    if (!NAT_IS_MODULE_OR_CLASS(parent)) {
        parent = NAT_OBJ_CLASS(parent);
        assert(parent);
    }

    Value *search_parent, *val;

    if (!strict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = parent;
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
        search_parent = parent;
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
    if (NAT_IS_MODULE_OR_CLASS(val) && !val->owner) {
        val->owner = parent;
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

static size_t hashmap_hash_identity(const void *key) {
    return (size_t)key;
}

static int hashmap_compare_identity(const void *a, const void *b) {
    return (size_t)a - (size_t)b;
}

GlobalEnv *build_global_env() {
    GlobalEnv *global_env = static_cast<GlobalEnv *>(malloc(sizeof(GlobalEnv)));
    struct hashmap *global_table = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
    hashmap_init(global_table, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(global_table, hashmap_alloc_key_string, free);
    global_env->globals = global_table;
    struct hashmap *symbol_table = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
    hashmap_init(symbol_table, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(symbol_table, hashmap_alloc_key_string, free);
    global_env->symbols = symbol_table;
    struct hashmap *heap_cells = static_cast<struct hashmap *>(malloc(sizeof(struct hashmap)));
    hashmap_init(heap_cells, hashmap_hash_identity, hashmap_compare_identity, 100);
    hashmap_set_key_alloc_funcs(heap_cells, NULL, NULL);
    global_env->heap_cells = heap_cells;
    global_env->heap = NULL;
    global_env->max_ptr = 0;
    global_env->min_ptr = reinterpret_cast<HeapCell *>(UINTPTR_MAX);
    global_env->bytes_available = global_env->bytes_total = 0;
    gc_alloc_heap_block(global_env);
    global_env->gc_enabled = false;
    return global_env;
}

void free_global_env(GlobalEnv *global_env) {
    hashmap_destroy(global_env->globals);
    hashmap_destroy(global_env->symbols);
    HeapBlock *block = global_env->heap;
    while (block) {
        HeapBlock *next_block = block->next;
        free(block);
        block = next_block;
    }
    free(global_env);
}

Env *build_env(Env *env, Env *outer) {
    memset(env, 0, sizeof(Env));
    env->outer = outer;
    if (outer) {
        env->global_env = outer->global_env;
    } else {
        env->global_env = build_global_env();
        env->global_env->top_env = env;
    }
    return env;
}

Env *build_block_env(Env *env, Env *outer, Env *calling_env) {
    build_env(env, outer);
    env->block_env = true;
    env->caller = calling_env;
    return env;
}

Env *build_detached_block_env(Env *env, Env *outer) {
    build_env(env, outer);
    env->block_env = true;
    env->outer = NULL;
    return env;
}

Value *exception_new(Env *env, Value *klass, const char *message) {
    Value *obj = alloc(env, klass, NAT_VALUE_EXCEPTION);
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

Value *raise(Env *env, Value *klass, const char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    Value *message = vsprintf(env, message_format, args);
    va_end(args);
    Value *exception = exception_new(env, klass, heap_string(message->str));
    raise_exception(env, exception);
    return exception;
}

Value *raise_exception(Env *env, Value *exception) {
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
    Value *exception = exception_new(env, const_get(env, NAT_OBJECT, "LocalJumpError", true), message);
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
        Value *klass_or_module = obj;
        if (NAT_TYPE(klass_or_module) != NAT_VALUE_CLASS && NAT_TYPE(klass_or_module) != NAT_VALUE_MODULE) {
            klass_or_module = NAT_OBJ_CLASS(klass_or_module);
        }
        NAT_RAISE(env, "NameError", "uninitialized class variable %s in %s", name, klass_or_module->class_name);
    }
}

Value *cvar_get_or_null(Env *env, Value *obj, const char *name) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    if (NAT_TYPE(obj) != NAT_VALUE_CLASS && NAT_TYPE(obj) != NAT_VALUE_MODULE) {
        obj = NAT_OBJ_CLASS(obj);
    }
    assert(NAT_TYPE(obj) == NAT_VALUE_CLASS || NAT_TYPE(obj) == NAT_VALUE_MODULE);

    Value *val = NULL;
    while (1) {
        if (obj->cvars.table) {
            val = static_cast<Value *>(hashmap_get(&obj->cvars, name));
            if (val) {
                return val;
            }
        }
        if (!obj->superclass) {
            return NULL;
        }
        obj = obj->superclass;
    }
}

Value *cvar_set(Env *env, Value *obj, const char *name, Value *val) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    if (NAT_TYPE(obj) != NAT_VALUE_CLASS && NAT_TYPE(obj) != NAT_VALUE_MODULE) {
        obj = NAT_OBJ_CLASS(obj);
    }
    assert(NAT_TYPE(obj) == NAT_VALUE_CLASS || NAT_TYPE(obj) == NAT_VALUE_MODULE);

    Value *current = obj;

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
            if (obj->cvars.table == NULL) {
                hashmap_init(&obj->cvars, hashmap_hash_string, hashmap_compare_string, 10);
                hashmap_set_key_alloc_funcs(&obj->cvars, hashmap_alloc_key_string, free);
            }
            hashmap_remove(&obj->cvars, name);
            hashmap_put(&obj->cvars, name, val);
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
    if (obj == NULL || NAT_TYPE(obj) == NAT_VALUE_FALSE || NAT_TYPE(obj) == NAT_VALUE_NIL) {
        return false;
    } else {
        return true;
    }
}

char *heap_string(const char *str) {
    return strdup(str);
}

static void init_class_or_module_data(Env *env, Value *klass_or_module) {
    hashmap_init(&klass_or_module->methods, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&klass_or_module->methods, hashmap_alloc_key_string, free);
    hashmap_init(&klass_or_module->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&klass_or_module->constants, hashmap_alloc_key_string, free);
    klass_or_module->cvars.table = NULL;
}

Value *subclass(Env *env, Value *superclass, const char *name) {
    assert(superclass);
    assert(NAT_OBJ_CLASS(superclass));
    Value *klass = alloc(env, NAT_OBJ_CLASS(superclass), NAT_VALUE_CLASS);
    if (superclass->singleton_class) {
        // TODO: what happens if the superclass gets a singleton_class later?
        klass->singleton_class = subclass(env, superclass->singleton_class, NULL);
    }
    klass->class_name = name ? heap_string(name) : NULL;
    klass->superclass = superclass;
    build_env(&klass->env, &superclass->env);
    klass->env.outer = NULL;
    init_class_or_module_data(env, klass);
    return klass;
}

Value *module(Env *env, const char *name) {
    Value *module = alloc(env, const_get(env, NAT_OBJECT, "Module", true), NAT_VALUE_MODULE);
    module->class_name = name ? heap_string(name) : NULL;
    build_env(&module->env, env);
    module->env.outer = NULL;
    init_class_or_module_data(env, module);
    return module;
}

void class_include(Env *env, Value *klass, Value *module) {
    klass->included_modules_count++;
    if (klass->included_modules_count == 1) {
        klass->included_modules_count++;
        klass->included_modules = static_cast<Value **>(calloc(2, sizeof(Value *)));
        klass->included_modules[0] = klass;
    } else {
        klass->included_modules = static_cast<Value **>(realloc(klass->included_modules, sizeof(Value *) * klass->included_modules_count));
    }
    klass->included_modules[klass->included_modules_count - 1] = module;
}

void class_prepend(Env *env, Value *klass, Value *module) {
    klass->included_modules_count++;
    if (klass->included_modules_count == 1) {
        klass->included_modules_count++;
        klass->included_modules = static_cast<Value **>(calloc(2, sizeof(Value *)));
        klass->included_modules[1] = klass;
    } else {
        klass->included_modules = static_cast<Value **>(realloc(klass->included_modules, sizeof(Value *) * klass->included_modules_count));
        for (ssize_t i = klass->included_modules_count - 1; i > 0; i--) {
            klass->included_modules[i] = klass->included_modules[i - 1];
        }
    }
    klass->included_modules[0] = module;
}

Value *initialize(Env *env, Value *obj, ssize_t argc, Value **args, Block *block) {
    Value *klass = NAT_OBJ_CLASS(obj);
    Value *matching_class_or_module;
    Method *method = find_method(klass, "initialize", &matching_class_or_module);
    if (method) {
        call_method_on_class(env, klass, klass, "initialize", obj, argc, args, block);
    }
    return obj;
}

Value *singleton_class(Env *env, Value *obj) {
    if (!obj->singleton_class) {
        obj->singleton_class = subclass(env, NAT_OBJ_CLASS(obj), NULL);
    }
    return obj->singleton_class;
}

Value *integer(Env *env, int64_t integer) {
    assert(integer >= NAT_MIN_INT);
    assert(integer <= NAT_MAX_INT);
    return (Value *)(integer << 1 | 1);
}

Value *symbol(Env *env, const char *name) {
    Value *symbol = static_cast<Value *>(hashmap_get(env->global_env->symbols, name));
    if (symbol) {
        return symbol;
    } else {
        symbol = alloc(env, const_get(env, NAT_OBJECT, "Symbol", true), NAT_VALUE_SYMBOL);
        symbol->symbol = heap_string(name);
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

Value *array_new(Env *env) {
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "Array", true), NAT_VALUE_ARRAY);
    vector_init(&obj->ary, 0);
    return obj;
}

Value *array_with_vals(Env *env, ssize_t count, ...) {
    va_list args;
    va_start(args, count);
    Value *ary = array_new(env);
    for (ssize_t i = 0; i < count; i++) {
        array_push(env, ary, va_arg(args, Value *));
    }
    va_end(args);
    return ary;
}

Value *array_copy(Env *env, Value *source) {
    assert(NAT_TYPE(source) == NAT_VALUE_ARRAY);
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "Array", true), NAT_VALUE_ARRAY);
    vector_copy(&obj->ary, &source->ary);
    return obj;
}

void array_push(Env *env, Value *array, Value *obj) {
    assert(NAT_TYPE(array) == NAT_VALUE_ARRAY);
    assert(obj);
    vector_push(&array->ary, obj);
}

void array_push_splat(Env *env, Value *array, Value *obj) {
    assert(NAT_TYPE(array) == NAT_VALUE_ARRAY);
    if (NAT_TYPE(obj) != NAT_VALUE_ARRAY && respond_to(env, obj, "to_a")) {
        obj = send(env, obj, "to_a", 0, NULL, NULL);
    }
    if (NAT_TYPE(obj) == NAT_VALUE_ARRAY) {
        for (ssize_t i = 0; i < vector_size(&obj->ary); i++) {
            vector_push(&array->ary, vector_get(&obj->ary, i));
        }
    } else {
        array_push(env, array, obj);
    }
}

void array_expand_with_nil(Env *env, Value *array, ssize_t size) {
    assert(NAT_TYPE(array) == NAT_VALUE_ARRAY);
    for (ssize_t i = vector_size(&array->ary); i < size; i++) {
        array_push(env, array, NAT_NIL);
    }
}

Value *splat(Env *env, Value *obj) {
    if (NAT_TYPE(obj) == NAT_VALUE_ARRAY) {
        return array_copy(env, obj);
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
    assert(NAT_TYPE(hash_obj) == NAT_VALUE_INTEGER);
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
    assert(NAT_TYPE(a_hash) == NAT_VALUE_INTEGER);
    assert(NAT_TYPE(b_hash) == NAT_VALUE_INTEGER);
    return NAT_INT_VALUE(a_hash) - NAT_INT_VALUE(b_hash);
}

HashKey *hash_key_list_append(Env *env, Value *hash, Value *key, Value *val) {
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
        build_detached_block_env(&new_last->env, env);
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
        build_detached_block_env(&node->env, env);
        node->env.caller = env;
        node->removed = false;
        hash->key_list = node;
        return node;
    }
}

void hash_key_list_remove_node(Value *hash, HashKey *node) {
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

HashIter *hash_iter(Env *env, Value *hash) {
    if (!hash->key_list) {
        return NULL;
    } else {
        hash->hash_is_iterating = true;
        return hash->key_list;
    }
}

HashIter *hash_iter_prev(Env *env, Value *hash, HashIter *iter) {
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

HashIter *hash_iter_next(Env *env, Value *hash, HashIter *iter) {
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

Value *hash_new(Env *env) {
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "Hash", true), NAT_VALUE_HASH);
    obj->key_list = NULL;
    obj->hash_default_value = NAT_NIL;
    obj->hash_default_block = NULL;
    hashmap_init(&obj->hashmap, hashmap_hash, hashmap_compare, 256);
    return obj;
}

Value *hash_get(Env *env, Value *hash, Value *key) {
    assert(NAT_TYPE(hash) == NAT_VALUE_HASH);
    HashKey key_container;
    key_container.key = key;
    key_container.env = *env;
    HashVal *container = static_cast<HashVal *>(hashmap_get(&hash->hashmap, &key_container));
    Value *val = container ? container->val : NULL;
    return val;
}

Value *hash_get_default(Env *env, Value *hash, Value *key) {
    if (hash->hash_default_block) {
        Value *args[2] = { hash, key };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, hash->hash_default_block, 2, args, NULL);
    } else {
        return hash->hash_default_value;
    }
}

void hash_put(Env *env, Value *hash, Value *key, Value *val) {
    assert(NAT_TYPE(hash) == NAT_VALUE_HASH);
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

Value *hash_delete(Env *env, Value *hash, Value *key) {
    assert(hash->type == NAT_VALUE_HASH);
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

Value *regexp(Env *env, const char *pattern) {
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
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "Regexp", true), NAT_VALUE_REGEXP);
    obj->regexp = regexp;
    obj->regexp_str = heap_string(pattern);
    return obj;
}

Value *matchdata(Env *env, OnigRegion *region, Value *str_obj) {
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "MatchData", true), NAT_VALUE_MATCHDATA);
    obj->matchdata_region = region;
    assert(NAT_TYPE(str_obj) == NAT_VALUE_STRING);
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
        free(hashmap_remove(&NAT_OBJ_CLASS(obj)->methods, name));
        hashmap_put(&NAT_OBJ_CLASS(obj)->methods, name, method);
    } else {
        free(hashmap_remove(&obj->methods, name));
        hashmap_put(&obj->methods, name, method);
    }
}

void define_method_with_block(Env *env, Value *obj, const char *name, Block *block) {
    Method *method = method_from_block(block);
    if (is_main_object(obj)) {
        free(hashmap_remove(&NAT_OBJ_CLASS(obj)->methods, name));
        hashmap_put(&NAT_OBJ_CLASS(obj)->methods, name, method);
    } else {
        free(hashmap_remove(&obj->methods, name));
        hashmap_put(&obj->methods, name, method);
    }
}

void define_singleton_method(Env *env, Value *obj, const char *name, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *block)) {
    Method *method = method_from_fn(fn);
    Value *klass = singleton_class(env, obj);
    free(hashmap_remove(&klass->methods, name));
    hashmap_put(&klass->methods, name, method);
}

void define_singleton_method_with_block(Env *env, Value *obj, const char *name, Block *block) {
    Method *method = method_from_block(block);
    Value *klass = singleton_class(env, obj);
    free(hashmap_remove(&klass->methods, name));
    hashmap_put(&klass->methods, name, method);
}

void undefine_method(Env *env, Value *obj, const char *name) {
    define_method(env, obj, name, NULL);
}

void undefine_singleton_method(Env *env, Value *obj, const char *name) {
    define_singleton_method(env, obj, name, NULL);
}

Value *class_ancestors(Env *env, Value *klass) {
    assert(NAT_TYPE(klass) == NAT_VALUE_CLASS || NAT_TYPE(klass) == NAT_VALUE_MODULE);
    Value *ancestors = array_new(env);
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
    if (obj == klass_or_module) {
        return true;
    } else {
        Value *ancestors = class_ancestors(env, NAT_OBJ_CLASS(obj));
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
    Value *klass;
    if (NAT_TYPE(receiver) == NAT_VALUE_INTEGER) {
        klass = NAT_INTEGER;
    } else {
        klass = receiver->singleton_class;
        if (klass) {
            Value *matching_class_or_module;
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
                    NAT_RAISE(env, "NoMethodError", "undefined method `%s' for %s:Class", sym, receiver->class_name);
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
void methods(Env *env, Value *array, Value *klass) {
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(&klass->methods); iter; iter = hashmap_iter_next(&klass->methods, iter)) {
        const char *name = (char *)hashmap_iter_get_key(iter);
        array_push(env, array, symbol(env, name));
    }
    for (ssize_t i = 0; i < klass->included_modules_count; i++) {
        Value *module = klass->included_modules[i];
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
Method *find_method(Value *klass, const char *method_name, Value **matching_class_or_module) {
    assert(NAT_TYPE(klass) == NAT_VALUE_CLASS);

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
        Value *module = klass->included_modules[i];
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

Method *find_method_without_undefined(Value *klass, const char *method_name, Value **matching_class_or_module) {
    Method *method = find_method(klass, method_name, matching_class_or_module);
    if (method && method->undefined) {
        return NULL;
    } else {
        return method;
    }
}

Value *call_method_on_class(Env *env, Value *klass, Value *instance_class, const char *method_name, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(klass != NULL);
    assert(NAT_TYPE(klass) == NAT_VALUE_CLASS);

    Value *matching_class_or_module;
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
        Env e;
        build_block_env(&e, closure_env, env);
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
    Env e;
    build_block_env(&e, env, env);
    return block_fn(&e, self);
}

bool respond_to(Env *env, Value *obj, const char *name) {
    Value *matching_class_or_module;
    if (NAT_TYPE(obj) == NAT_VALUE_INTEGER) {
        Value *klass = NAT_INTEGER;
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
    Env e;
    build_block_env(&e, &the_block->env, env);
    return the_block->fn(&e, the_block->self, argc, args, block);
}

Value *proc_new(Env *env, Block *block) {
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "Proc", true), NAT_VALUE_PROC);
    obj->block = block;
    return obj;
}

Value *to_proc(Env *env, Value *obj) {
    if (NAT_TYPE(obj) == NAT_VALUE_PROC) {
        return obj;
    } else if (respond_to(env, obj, "to_proc")) {
        return send(env, obj, "to_proc", 0, NULL, NULL);
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Proc)", NAT_OBJ_CLASS(obj)->class_name);
    }
}

Value *lambda(Env *env, Block *block) {
    Value *lambda = proc_new(env, block);
    lambda->lambda = true;
    return lambda;
}

Value *string_n(Env *env, const char *str, ssize_t len) {
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "String", true), NAT_VALUE_STRING);
    char *copy = static_cast<char *>(malloc(len + 1));
    memcpy(copy, str, len);
    obj->str = copy;
    obj->str[len] = 0;
    obj->str_len = len;
    obj->str_cap = len;
    obj->encoding = NAT_ENCODING_UTF_8; // TODO: inherit from encoding of file?
    return obj;
}

Value *string(Env *env, const char *str) {
    ssize_t len = strlen(str);
    return string_n(env, str, len);
}

void grow_string(Env *env, Value *obj, ssize_t capacity) {
    ssize_t len = strlen(obj->str);
    assert(capacity >= len);
    obj->str = static_cast<char *>(realloc(obj->str, capacity + 1));
    obj->str_cap = capacity;
}

void grow_string_at_least(Env *env, Value *obj, ssize_t min_capacity) {
    ssize_t capacity = obj->str_cap;
    if (capacity >= min_capacity)
        return;
    if (capacity > 0 && min_capacity <= capacity * NAT_STRING_GROW_FACTOR) {
        grow_string(env, obj, capacity * NAT_STRING_GROW_FACTOR);
    } else {
        grow_string(env, obj, min_capacity);
    }
}

void string_append(Env *env, Value *str, const char *s) {
    ssize_t new_len = strlen(s);
    if (new_len == 0) return;
    ssize_t total_len = str->str_len + new_len;
    grow_string_at_least(env, str, total_len);
    strcat(str->str, s);
    str->str_len = total_len;
}

void string_append_char(Env *env, Value *str, char c) {
    ssize_t total_len = str->str_len + 1;
    grow_string_at_least(env, str, total_len);
    str->str[total_len - 1] = c;
    str->str[total_len] = 0;
    str->str_len = total_len;
}

void string_append_string(Env *env, Value *str, Value *str2) {
    if (str2->str_len == 0) return;
    ssize_t total_len = str->str_len + str2->str_len;
    grow_string_at_least(env, str, total_len);
    strncat(str->str, str2->str, str2->str_len);
    str->str_len = total_len;
}

#define NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, message_format, ...)                          \
    {                                                                                                     \
        Value *Encoding = const_get(env, NAT_OBJECT, "Encoding", true);                               \
        Value *InvalidByteSequenceError = const_get(env, Encoding, "InvalidByteSequenceError", true); \
        raise(env, InvalidByteSequenceError, message_format, ##__VA_ARGS__);                              \
    }

Value *string_chars(Env *env, Value *str) {
    Value *ary = array_new(env);
    Value *c;
    char buffer[5];
    switch (str->encoding) {
    case NAT_ENCODING_UTF_8:
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
            c->encoding = NAT_ENCODING_UTF_8;
            array_push(env, ary, c);
        }
        break;
    case NAT_ENCODING_ASCII_8BIT:
        for (ssize_t i = 0; i < str->str_len; i++) {
            buffer[0] = str->str[i];
            buffer[1] = 0;
            c = string(env, buffer);
            c->encoding = NAT_ENCODING_ASCII_8BIT;
            array_push(env, ary, c);
        }
        break;
    }
    return ary;
}

Value *sprintf(Env *env, const char *format, ...) {
    va_list args;
    va_start(args, format);
    Value *out = vsprintf(env, format, args);
    va_end(args);
    return out;
}

Value *vsprintf(Env *env, const char *format, va_list args) {
    Value *out = string(env, "");
    ssize_t len = strlen(format);
    Value *inspected;
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
                string_append_string(env, out, va_arg(args, Value *));
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
                inspected = send(env, va_arg(args, Value *), "inspect", 0, NULL, NULL);
                assert(NAT_TYPE(inspected) == NAT_VALUE_STRING);
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

Value *range(Env *env, Value *begin, Value *end, bool exclude_end) {
    Value *obj = alloc(env, const_get(env, NAT_OBJECT, "Range", true), NAT_VALUE_RANGE);
    obj->range_begin = begin;
    obj->range_end = end;
    obj->range_exclude_end = exclude_end;
    return obj;
}

Value *dup(Env *env, Value *obj) {
    Value *copy = NULL;
    switch (NAT_TYPE(obj)) {
    case NAT_VALUE_ARRAY:
        copy = array_new(env);
        for (ssize_t i = 0; i < vector_size(&obj->ary); i++) {
            array_push(env, copy, static_cast<Value *>(vector_get(&obj->ary, i)));
        }
        return copy;
    case NAT_VALUE_STRING:
        return string(env, obj->str);
    case NAT_VALUE_SYMBOL:
        return symbol(env, obj->symbol);
    case NAT_VALUE_FALSE:
    case NAT_VALUE_NIL:
    case NAT_VALUE_TRUE:
        return obj;
    default:
        fprintf(stderr, "I don't know how to dup this kind of object yet %d.\n", obj->type);
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
    if (NAT_TYPE(self) == NAT_VALUE_INTEGER || NAT_TYPE(self) == NAT_VALUE_SYMBOL) {
        NAT_RAISE(env, "TypeError", "no klass to make alias");
    }
    if (is_main_object(self)) {
        self = NAT_OBJ_CLASS(self);
    }
    Value *klass = self;
    if (NAT_TYPE(self) != NAT_VALUE_CLASS && NAT_TYPE(self) != NAT_VALUE_MODULE) {
        klass = singleton_class(env, self);
    }
    Value *matching_class_or_module;
    Method *method = find_method(klass, old_name, &matching_class_or_module);
    if (!method) {
        NAT_RAISE(env, "NameError", "undefined method `%s' for `%v'", old_name, klass);
    }
    free(hashmap_remove(&klass->methods, new_name));
    Method *method_copy = static_cast<Method *>(malloc(sizeof(Method)));
    memcpy(method_copy, method, sizeof(Method));
    hashmap_put(&klass->methods, new_name, method_copy);
}

void run_at_exit_handlers(Env *env) {
    Value *at_exit_handlers = global_get(env, "$NAT_at_exit_handlers");
    assert(at_exit_handlers);
    for (int i = vector_size(&at_exit_handlers->ary) - 1; i >= 0; i--) {
        Value *proc = static_cast<Value *>(vector_get(&at_exit_handlers->ary, i));
        assert(proc);
        assert(NAT_TYPE(proc) == NAT_VALUE_PROC);
        NAT_RUN_BLOCK_WITHOUT_BREAK(env, proc->block, 0, NULL, NULL);
    }
}

void print_exception_with_backtrace(Env *env, Value *exception) {
    assert(NAT_TYPE(exception) == NAT_VALUE_EXCEPTION);
    Value *stderr = global_get(env, "$stderr");
    int fd = stderr->fileno;
    if (vector_size(&exception->backtrace->ary) > 0) {
        dprintf(fd, "Traceback (most recent call last):\n");
        for (int i = vector_size(&exception->backtrace->ary) - 1; i > 0; i--) {
            Value *line = static_cast<Value *>(vector_get(&exception->backtrace->ary, i));
            assert(NAT_TYPE(line) == NAT_VALUE_STRING);
            dprintf(fd, "        %d: from %s\n", i, line->str);
        }
        Value *line = static_cast<Value *>(vector_get(&exception->backtrace->ary, 0));
        dprintf(fd, "%s: ", line->str);
    }
    dprintf(fd, "%s (%s)\n", exception->message, NAT_OBJ_CLASS(exception)->class_name);
}

void handle_top_level_exception(Env *env, bool run_exit_handlers) {
    Value *exception = env->exception;
    assert(exception);
    assert(NAT_TYPE(exception) == NAT_VALUE_EXCEPTION);
    env->rescue = false;
    if (is_a(env, exception, const_get(env, NAT_OBJECT, "SystemExit", true))) {
        Value *status_obj = ivar_get(env, exception, "@status");
        if (run_exit_handlers) run_at_exit_handlers(env);
        if (NAT_TYPE(status_obj) == NAT_VALUE_INTEGER) {
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
    if (NAT_TYPE(obj) == NAT_VALUE_INTEGER) {
        return (int64_t)obj;
    } else {
        return (int64_t)obj / 2;
    }
}

Value *convert_to_real_object(Env *env, Value *obj) {
    if (((int64_t)obj & 1)) {
        Value *real_obj = alloc(env, NAT_INTEGER, NAT_VALUE_INTEGER);
        real_obj->integer = NAT_INT_VALUE(obj);
        return real_obj;
    } else {
        return obj;
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

Value *to_ary(Env *env, Value *obj, bool raise_for_non_array) {
    if (NAT_TYPE(obj) == NAT_VALUE_ARRAY) {
        return obj;
    } else if (respond_to(env, obj, "to_ary")) {
        Value *ary = send(env, obj, "to_ary", 0, NULL, NULL);
        if (NAT_TYPE(ary) == NAT_VALUE_ARRAY) {
            return ary;
        } else if (NAT_TYPE(ary) == NAT_VALUE_NIL || !raise_for_non_array) {
            ary = array_new(env);
            array_push(env, ary, obj);
            return ary;
        } else {
            const char *class_name = NAT_OBJ_CLASS(obj)->class_name;
            NAT_RAISE(env, "TypeError", "can't convert %s to Array (%s#to_ary gives %s)", class_name, class_name, NAT_OBJ_CLASS(ary)->class_name);
        }
    } else {
        Value *ary = array_new(env);
        array_push(env, ary, obj);
        return ary;
    }
}

static Value *splat_value(Env *env, Value *value, ssize_t index, ssize_t offset_from_end) {
    Value *splat = array_new(env);
    if (NAT_TYPE(value) == NAT_VALUE_ARRAY && index < vector_size(&value->ary) - offset_from_end) {
        for (ssize_t s = index; s < vector_size(&value->ary) - offset_from_end; s++) {
            array_push(env, splat, static_cast<Value *>(vector_get(&value->ary, s)));
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
            if (NAT_TYPE(return_value) == NAT_VALUE_ARRAY) {

                int64_t ary_len = (int64_t)vector_size(&return_value->ary);

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
                    return_value = static_cast<Value *>(vector_get(&return_value->ary, index));

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
            if (NAT_TYPE(return_value) == NAT_VALUE_ARRAY) {

                int64_t ary_len = (int64_t)vector_size(&return_value->ary);

                if (index < 0) {
                    // negative offset index should go from the right
                    index = ary_len + index;
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = static_cast<Value *>(vector_get(&return_value->ary, index));

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
    assert(NAT_TYPE(args) == NAT_VALUE_ARRAY);
    Value *hash;
    if (vector_size(&args->ary) == 0) {
        hash = hash_new(env);
    } else {
        hash = static_cast<Value *>(vector_get(&args->ary, vector_size(&args->ary) - 1));
        if (NAT_TYPE(hash) != NAT_VALUE_HASH) {
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

Value *args_to_array(Env *env, ssize_t argc, Value **args) {
    Value *ary = array_new(env);
    for (ssize_t i = 0; i < argc; i++) {
        array_push(env, ary, args[i]);
    }
    return ary;
}

// much like args_to_array above, but when a block is given a single arg,
// and the block wants multiple args, call to_ary on the first arg and return that
Value *block_args_to_array(Env *env, ssize_t signature_size, ssize_t argc, Value **args) {
    if (argc == 1 && signature_size > 1) {
        return to_ary(env, args[0], true);
    }
    return args_to_array(env, argc, args);
}

Value *encoding(Env *env, enum NatEncoding num, Value *names) {
    Value *encoding = alloc(env, const_get(env, NAT_OBJECT, "Encoding", true), NAT_VALUE_ENCODING);
    encoding->encoding_num = num;
    encoding->encoding_names = names;
    freeze_object(names);
    return encoding;
}

Value *eval_class_or_module_body(Env *env, Value *class_or_module, Value *(*fn)(Env *, Value *)) {
    Env body_env;
    build_env(&body_env, env);
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
            NAT_ASSERT_TYPE(obj, NAT_VALUE_INTEGER, "Integer");
            *int_ptr = NAT_INT_VALUE(obj);
            break;
        case 's':
            str_ptr = va_arg(va_args, char **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            if (obj == NAT_NIL) {
                *str_ptr = NULL;
            } else {
                NAT_ASSERT_TYPE(obj, NAT_VALUE_STRING, "String");
            }
            *str_ptr = obj->str;
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
            assert(obj->type == NAT_VALUE_VOIDP);
            *void_ptr = obj->void_ptr;
            break;
        default:
            fprintf(stderr, "Unknown arg spread arrangement specifier: %%%c", c);
            abort();
        }
    }
    va_end(va_args);
}

Value *void_ptr(Env *env, void *ptr) {
    Value *obj = alloc(env, NAT_OBJECT, NAT_VALUE_VOIDP);
    obj->void_ptr = ptr;
    return obj;
}

}
