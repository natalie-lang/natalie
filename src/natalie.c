#include "natalie.h"
#include "gc.h"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

bool nat_is_constant_name(char *name) {
    return strlen(name) > 0 && isupper(name[0]);
}

bool nat_is_global_name(char *name) {
    return strlen(name) > 0 && name[0] == '$';
}

NatObject *nat_const_get(NatEnv *env, NatObject *parent, char *name, bool strict) {
    NatObject *val = nat_const_get_or_null(env, parent, name, strict, false);
    if (val) {
        return val;
    } else if (strict) {
        NAT_RAISE(env, "NameError", "uninitialized constant %S::%s", nat_send(env, parent, "inspect", 0, NULL, NULL), name);
    } else {
        NAT_RAISE(env, "NameError", "uninitialized constant %s", name);
    }
}

NatObject *nat_const_get_or_null(NatEnv *env, NatObject *parent, char *name, bool strict, bool define) {
    if (!NAT_IS_MODULE_OR_CLASS(parent)) {
        parent = NAT_OBJ_CLASS(parent);
        assert(parent);
    }

    NatObject *search_parent, *val;

    if (!strict) {
        // first search in parent namespaces (not including global, i.e. Object namespace)
        search_parent = parent;
        while (!(val = hashmap_get(&search_parent->constants, name)) && search_parent->owner && search_parent->owner != NAT_OBJECT) {
            search_parent = search_parent->owner;
        }
        if (val) return val;
    }

    if (define) {
        // don't search superclasses
        val = hashmap_get(&parent->constants, name);
        if (val) return val;
    } else {
        // search in superclass hierarchy
        search_parent = parent;
        while (!(val = hashmap_get(&search_parent->constants, name)) && search_parent->superclass) {
            search_parent = search_parent->superclass;
        }
        if (val) return val;
    }

    if (!strict) {
        // lastly, search on the global, i.e. Object namespace
        val = hashmap_get(&NAT_OBJECT->constants, name);
        if (val) return val;
    }

    return NULL;
}

NatObject *nat_const_set(NatEnv *env, NatObject *parent, char *name, NatObject *val) {
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

NatObject *nat_var_get(NatEnv *env, char *key, ssize_t index) {
    if (index >= env->var_count) {
        printf("Trying to get variable `%s' at index %zu which is not set (env->vars = %p, env->var_count = %zu).\n", key, index, env->vars, env->var_count);
        abort();
    }
    NatObject *val = env->vars[index];
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

NatObject *nat_var_set(NatEnv *env, char *name, ssize_t index, bool allocate, NatObject *val) {
    ssize_t needed = index + 1;
    if (needed > env->var_count && allocate) {
        if (env->var_count == 0) {
            env->vars = calloc(needed, sizeof(NatObject *));
            env->var_count = needed;
        } else {
            env->vars = realloc(env->vars, sizeof(NatObject *) * needed);
            env->var_count = needed;
        }
    }
    env->vars[index] = val;
    return val;
}

static size_t hashmap_hash_identity(const void *key) {
    return (size_t)key;
}

static int hashmap_compare_identity(const void *a, const void *b) {
    return (size_t)a - (size_t)b;
}

NatGlobalEnv *nat_build_global_env() {
    NatGlobalEnv *global_env = malloc(sizeof(NatGlobalEnv));
    struct hashmap *global_table = malloc(sizeof(struct hashmap));
    hashmap_init(global_table, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(global_table, hashmap_alloc_key_string, free);
    global_env->globals = global_table;
    struct hashmap *symbol_table = malloc(sizeof(struct hashmap));
    hashmap_init(symbol_table, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(symbol_table, hashmap_alloc_key_string, free);
    global_env->symbols = symbol_table;
    struct hashmap *heap_cells = malloc(sizeof(struct hashmap));
    hashmap_init(heap_cells, hashmap_hash_identity, hashmap_compare_identity, 100);
    hashmap_set_key_alloc_funcs(heap_cells, NULL, NULL);
    global_env->heap_cells = heap_cells;
    global_env->heap = NULL;
    global_env->max_ptr = 0;
    global_env->min_ptr = (void *)UINTPTR_MAX;
    global_env->bytes_available = global_env->bytes_total = 0;
    nat_gc_alloc_heap_block(global_env);
    global_env->gc_enabled = false;
    return global_env;
}

void nat_free_global_env(NatGlobalEnv *global_env) {
    hashmap_destroy(global_env->globals);
    hashmap_destroy(global_env->symbols);
    NatHeapBlock *block = global_env->heap;
    while (block) {
        NatHeapBlock *next_block = block->next;
        free(block);
        block = next_block;
    }
    free(global_env);
}

NatEnv *nat_build_env(NatEnv *env, NatEnv *outer) {
    memset(env, 0, sizeof(NatEnv));
    env->outer = outer;
    if (outer) {
        env->global_env = outer->global_env;
    } else {
        env->global_env = nat_build_global_env();
    }
    return env;
}

NatEnv *nat_build_block_env(NatEnv *env, NatEnv *outer, NatEnv *calling_env) {
    nat_build_env(env, outer);
    env->block_env = true;
    env->caller = calling_env;
    return env;
}

NatEnv *nat_build_detached_block_env(NatEnv *env, NatEnv *outer) {
    nat_build_env(env, outer);
    env->block_env = true;
    env->outer = NULL;
    return env;
}

char *nat_find_current_method_name(NatEnv *env) {
    while ((!env->method_name || strcmp(env->method_name, "<block>") == 0) && env->outer) {
        env = env->outer;
    }
    if (strcmp(env->method_name, "<main>") == 0) return NULL;
    return env->method_name;
}

// note: returns a heap pointer that the caller must free
char *nat_build_code_location_name(NatEnv *env) {
    do {
        if (env->method_name) {
            if (strcmp(env->method_name, "<block>") == 0) {
                char *outer_name = nat_build_code_location_name(env->outer);
                char *name = heap_string(nat_sprintf(env, "block in %s", outer_name)->str);
                free(outer_name);
                return name;
            } else {
                return heap_string(env->method_name);
            }
        }
        env = env->outer;
    } while (env);
    return heap_string("(unknown)");
}

NatObject *nat_raise(NatEnv *env, NatObject *klass, char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    NatObject *message = nat_vsprintf(env, message_format, args);
    va_end(args);
    NatObject *exception = nat_exception(env, klass, heap_string(message->str));
    nat_raise_exception(env, exception);
    return exception;
}

NatObject *nat_raise_exception(NatEnv *env, NatObject *exception) {
    if (!exception->backtrace) { // only build a backtrace the first time the exception is raised (not on a re-raise)
        NatObject *bt = exception->backtrace = nat_array(env);
        NatEnv *bt_env = env;
        do {
            if (bt_env->file) {
                char *method_name = nat_build_code_location_name(bt_env);
                nat_array_push(env, bt, nat_sprintf(env, "%s:%d:in `%s'", bt_env->file, bt_env->line, method_name));
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

NatObject *nat_raise_local_jump_error(NatEnv *env, NatObject *exit_value, char *message) {
    NatObject *exception = nat_exception(env, nat_const_get(env, NAT_OBJECT, "LocalJumpError", true), message);
    nat_ivar_set(env, exception, "@exit_value", exit_value);
    nat_raise_exception(env, exception);
    return exception;
}

NatObject *nat_ivar_get(NatEnv *env, NatObject *obj, char *name) {
    assert(strlen(name) > 0);
    if (name[0] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an instance variable name", name);
    }
    if (obj->ivars.table == NULL) {
        hashmap_init(&obj->ivars, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(&obj->ivars, hashmap_alloc_key_string, free);
    }
    NatObject *val = hashmap_get(&obj->ivars, name);
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

NatObject *nat_ivar_set(NatEnv *env, NatObject *obj, char *name, NatObject *val) {
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

NatObject *nat_cvar_get(NatEnv *env, NatObject *obj, char *name) {
    NatObject *val = nat_cvar_get_or_null(env, obj, name);
    if (val) {
        return val;
    } else {
        NatObject *klass_or_module = obj;
        if (NAT_TYPE(klass_or_module) != NAT_VALUE_CLASS && NAT_TYPE(klass_or_module) != NAT_VALUE_MODULE) {
            klass_or_module = NAT_OBJ_CLASS(klass_or_module);
        }
        NAT_RAISE(env, "NameError", "uninitialized class variable %s in %s", name, klass_or_module->class_name);
    }
}

NatObject *nat_cvar_get_or_null(NatEnv *env, NatObject *obj, char *name) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    if (NAT_TYPE(obj) != NAT_VALUE_CLASS && NAT_TYPE(obj) != NAT_VALUE_MODULE) {
        obj = NAT_OBJ_CLASS(obj);
    }
    assert(NAT_TYPE(obj) == NAT_VALUE_CLASS || NAT_TYPE(obj) == NAT_VALUE_MODULE);

    NatObject *val = NULL;
    while (1) {
        if (obj->cvars.table) {
            val = hashmap_get(&obj->cvars, name);
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

NatObject *nat_cvar_set(NatEnv *env, NatObject *obj, char *name, NatObject *val) {
    assert(strlen(name) > 1);
    if (name[0] != '@' || name[1] != '@') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a class variable name", name);
    }

    if (NAT_TYPE(obj) != NAT_VALUE_CLASS && NAT_TYPE(obj) != NAT_VALUE_MODULE) {
        obj = NAT_OBJ_CLASS(obj);
    }
    assert(NAT_TYPE(obj) == NAT_VALUE_CLASS || NAT_TYPE(obj) == NAT_VALUE_MODULE);

    NatObject *current = obj;

    NatObject *exists = NULL;
    while (1) {
        if (current->cvars.table) {
            exists = hashmap_get(&current->cvars, name);
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

NatObject *nat_global_get(NatEnv *env, char *name) {
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as a global variable name", name);
    }
    NatObject *val = hashmap_get(env->global_env->globals, name);
    if (val) {
        return val;
    } else {
        return NAT_NIL;
    }
}

NatObject *nat_global_set(NatEnv *env, char *name, NatObject *val) {
    assert(strlen(name) > 0);
    if (name[0] != '$') {
        NAT_RAISE(env, "NameError", "`%s' is not allowed as an global variable name", name);
    }
    hashmap_remove(env->global_env->globals, name);
    hashmap_put(env->global_env->globals, name, val);
    return val;
}

bool nat_truthy(NatObject *obj) {
    if (obj == NULL || NAT_TYPE(obj) == NAT_VALUE_FALSE || NAT_TYPE(obj) == NAT_VALUE_NIL) {
        return false;
    } else {
        return true;
    }
}

char *heap_string(char *str) {
    ssize_t len = strlen(str);
    char *copy = malloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

static void nat_init_class_or_module_data(NatEnv *env, NatObject *klass_or_module) {
    hashmap_init(&klass_or_module->methods, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&klass_or_module->methods, hashmap_alloc_key_string, free);
    hashmap_init(&klass_or_module->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&klass_or_module->constants, hashmap_alloc_key_string, free);
    klass_or_module->cvars.table = NULL;
}

NatObject *nat_subclass(NatEnv *env, NatObject *superclass, char *name) {
    assert(superclass);
    assert(NAT_OBJ_CLASS(superclass));
    NatObject *klass = nat_alloc(env, NAT_OBJ_CLASS(superclass), NAT_VALUE_CLASS);
    if (superclass->singleton_class) {
        // TODO: what happens if the superclass gets a singleton_class later?
        klass->singleton_class = nat_subclass(env, superclass->singleton_class, NULL);
    }
    klass->class_name = name ? heap_string(name) : NULL;
    klass->superclass = superclass;
    nat_build_env(&klass->env, &superclass->env);
    klass->env.outer = NULL;
    nat_init_class_or_module_data(env, klass);
    return klass;
}

NatObject *nat_module(NatEnv *env, char *name) {
    NatObject *module = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Module", true), NAT_VALUE_MODULE);
    module->class_name = name ? heap_string(name) : NULL;
    nat_build_env(&module->env, env);
    module->env.outer = NULL;
    nat_init_class_or_module_data(env, module);
    return module;
}

void nat_class_include(NatEnv *env, NatObject *klass, NatObject *module) {
    klass->included_modules_count++;
    if (klass->included_modules_count == 1) {
        klass->included_modules_count++;
        klass->included_modules = calloc(2, sizeof(NatObject *));
        klass->included_modules[0] = klass;
    } else {
        klass->included_modules = realloc(klass->included_modules, sizeof(NatObject *) * klass->included_modules_count);
    }
    klass->included_modules[klass->included_modules_count - 1] = module;
}

void nat_class_prepend(NatEnv *env, NatObject *klass, NatObject *module) {
    klass->included_modules_count++;
    if (klass->included_modules_count == 1) {
        klass->included_modules_count++;
        klass->included_modules = calloc(2, sizeof(NatObject *));
        klass->included_modules[1] = klass;
    } else {
        klass->included_modules = realloc(klass->included_modules, sizeof(NatObject *) * klass->included_modules_count);
        for (ssize_t i = klass->included_modules_count - 1; i > 0; i--) {
            klass->included_modules[i] = klass->included_modules[i - 1];
        }
    }
    klass->included_modules[0] = module;
}

NatObject *nat_initialize(NatEnv *env, NatObject *obj, ssize_t argc, NatObject **args, NatBlock *block) {
    NatObject *klass = NAT_OBJ_CLASS(obj);
    NatObject *matching_class_or_module;
    NatMethod *method = nat_find_method(klass, "initialize", &matching_class_or_module);
    if (method) {
        nat_call_method_on_class(env, klass, klass, "initialize", obj, argc, args, block);
    }
    return obj;
}

NatObject *nat_singleton_class(NatEnv *env, NatObject *obj) {
    if (!obj->singleton_class) {
        obj->singleton_class = nat_subclass(env, NAT_OBJ_CLASS(obj), NULL);
    }
    return obj->singleton_class;
}

NatObject *nat_integer(NatEnv *env, int64_t integer) {
    assert(integer >= NAT_MIN_INT);
    assert(integer <= NAT_MAX_INT);
    return (NatObject *)(integer << 1 | 1);
}

NatObject *nat_symbol(NatEnv *env, char *name) {
    NatObject *symbol = hashmap_get(env->global_env->symbols, name);
    if (symbol) {
        return symbol;
    } else {
        symbol = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Symbol", true), NAT_VALUE_SYMBOL);
        symbol->symbol = heap_string(name);
        hashmap_put(env->global_env->symbols, name, symbol);
        return symbol;
    }
}

NatObject *nat_exception(NatEnv *env, NatObject *klass, char *message) {
    NatObject *obj = nat_alloc(env, klass, NAT_VALUE_EXCEPTION);
    assert(message);
    NatObject *message_obj = nat_string(env, message);
    nat_initialize(env, obj, 1, &message_obj, NULL);
    return obj;
}

void nat_vector_init(NatVector *vec, ssize_t size) {
    vec->size = size;
    vec->capacity = size;
    if (size > 0) {
        vec->data = calloc(size, sizeof(void*));
    } else {
        vec->data = NULL;
    }
}

inline ssize_t nat_vector_size(NatVector *vec) {
    return vec->size;
}

inline void **nat_vector_data(NatVector *vec) {
    return vec->data;
}

inline void *nat_vector_get(NatVector *vec, ssize_t index) {
    return vec->data[index];
}

inline void nat_vector_set(NatVector *vec, ssize_t index, void *item) {
    vec->data[index] = item;
}

inline void nat_vector_free(NatVector *vec) {
    free(vec->data);
}

void nat_vector_copy(NatVector *dest, NatVector *source) {
    nat_vector_init(dest, source->capacity);
    dest->size = source->size;
    memcpy(dest->data, source->data, source->size * sizeof(void *));
}

static void nat_vector_grow(NatVector *vec, ssize_t capacity) {
    vec->data = realloc(vec->data, sizeof(void *) * capacity);
    vec->capacity = capacity;
}

static void nat_vector_grow_at_least(NatVector *vec, ssize_t min_capacity) {
    ssize_t capacity = vec->capacity;
    if (capacity >= min_capacity) {
        return;
    }
    if (capacity > 0 && min_capacity <= capacity * NAT_VECTOR_GROW_FACTOR) {
        nat_vector_grow(vec, capacity * NAT_VECTOR_GROW_FACTOR);
    } else {
        nat_vector_grow(vec, min_capacity);
    }
}

void nat_vector_push(NatVector *vec, void *item) {
    ssize_t len = vec->size;
    if (vec->size >= vec->capacity) {
        nat_vector_grow_at_least(vec, len + 1);
    }
    vec->size++;
    vec->data[len] = item;
}

void nat_vector_add(NatVector *new_vec, NatVector *vec1, NatVector *vec2) {
    nat_vector_grow_at_least(new_vec, vec1->size + vec2->size);
    memcpy(new_vec->data, vec1->data, vec1->size * sizeof(void *));
    memcpy(new_vec->data + vec1->size, vec2->data, vec2->size * sizeof(void *));
    new_vec->size = vec1->size + vec2->size;
}

NatObject *nat_array(NatEnv *env) {
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Array", true), NAT_VALUE_ARRAY);
    nat_vector_init(&obj->ary, 0);
    return obj;
}

NatObject *nat_array_with_vals(NatEnv *env, ssize_t count, ...) {
    va_list args;
    va_start(args, count);
    NatObject *ary = nat_array(env);
    for (ssize_t i = 0; i < count; i++) {
        nat_array_push(env, ary, va_arg(args, NatObject *));
    }
    va_end(args);
    return ary;
}

NatObject *nat_array_copy(NatEnv *env, NatObject *source) {
    assert(NAT_TYPE(source) == NAT_VALUE_ARRAY);
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Array", true), NAT_VALUE_ARRAY);
    nat_vector_copy(&obj->ary, &source->ary);
    return obj;
}

void nat_array_push(NatEnv *env, NatObject *array, NatObject *obj) {
    assert(NAT_TYPE(array) == NAT_VALUE_ARRAY);
    assert(obj);
    nat_vector_push(&array->ary, obj);
}

void nat_array_push_splat(NatEnv *env, NatObject *array, NatObject *obj) {
    assert(NAT_TYPE(array) == NAT_VALUE_ARRAY);
    if (NAT_TYPE(obj) != NAT_VALUE_ARRAY && nat_respond_to(env, obj, "to_a")) {
        obj = nat_send(env, obj, "to_a", 0, NULL, NULL);
    }
    if (NAT_TYPE(obj) == NAT_VALUE_ARRAY) {
        for (ssize_t i = 0; i < nat_vector_size(&obj->ary); i++) {
            nat_vector_push(&array->ary, nat_vector_get(&obj->ary, i));
        }
    } else {
        nat_array_push(env, array, obj);
    }
}

void nat_array_expand_with_nil(NatEnv *env, NatObject *array, ssize_t size) {
    assert(NAT_TYPE(array) == NAT_VALUE_ARRAY);
    for (ssize_t i = nat_vector_size(&array->ary); i < size; i++) {
        nat_array_push(env, array, NAT_NIL);
    }
}

NatObject *nat_splat(NatEnv *env, NatObject *obj) {
    if (NAT_TYPE(obj) == NAT_VALUE_ARRAY) {
        return nat_array_copy(env, obj);
    } else {
        return nat_to_ary(env, obj, false);
    }
}

// this is used by the hashmap library and assumes that obj->env has been set
size_t nat_hashmap_hash(const void *key) {
    NatHashKey *key_p = (NatHashKey *)key;
    assert(NAT_OBJ_HAS_ENV2(key_p));
    NatObject *hash_obj = nat_send(&key_p->env, key_p->key, "hash", 0, NULL, NULL);
    assert(NAT_TYPE(hash_obj) == NAT_VALUE_INTEGER);
    return NAT_INT_VALUE(hash_obj);
}

// this is used by the hashmap library to compare keys
int nat_hashmap_compare(const void *a, const void *b) {
    NatHashKey *a_p = (NatHashKey *)a;
    NatHashKey *b_p = (NatHashKey *)b;
    assert(NAT_OBJ_HAS_ENV2(a_p));
    assert(NAT_OBJ_HAS_ENV2(b_p));
    NatObject *a_hash = nat_send(&a_p->env, a_p->key, "hash", 0, NULL, NULL);
    NatObject *b_hash = nat_send(&b_p->env, b_p->key, "hash", 0, NULL, NULL);
    assert(NAT_TYPE(a_hash) == NAT_VALUE_INTEGER);
    assert(NAT_TYPE(b_hash) == NAT_VALUE_INTEGER);
    return NAT_INT_VALUE(a_hash) - NAT_INT_VALUE(b_hash);
}

NatHashKey *nat_hash_key_list_append(NatEnv *env, NatObject *hash, NatObject *key, NatObject *val) {
    if (hash->key_list) {
        NatHashKey *first = hash->key_list;
        NatHashKey *last = hash->key_list->prev;
        NatHashKey *new_last = malloc(sizeof(NatHashKey));
        new_last->key = key;
        new_last->val = val;
        // <first> ... <last> <new_last> -|
        // ^______________________________|
        new_last->prev = last;
        new_last->next = first;
        nat_build_detached_block_env(&new_last->env, env);
        new_last->removed = false;
        first->prev = new_last;
        last->next = new_last;
        return new_last;
    } else {
        NatHashKey *node = malloc(sizeof(NatHashKey));
        node->key = key;
        node->val = val;
        node->prev = node;
        node->next = node;
        nat_build_detached_block_env(&node->env, env);
        node->removed = false;
        hash->key_list = node;
        return node;
    }
}

void nat_hash_key_list_remove_node(NatObject *hash, NatHashKey *node) {
    NatHashKey *prev = node->prev;
    NatHashKey *next = node->next;
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

NatHashIter *nat_hash_iter(NatEnv *env, NatObject *hash) {
    if (!hash->key_list) {
        return NULL;
    } else {
        hash->hash_is_iterating = true;
        return hash->key_list;
    }
}

NatHashIter *nat_hash_iter_prev(NatEnv *env, NatObject *hash, NatHashIter *iter) {
    if (iter->prev == NULL || iter == hash->key_list) {
        // finished
        hash->hash_is_iterating = false;
        return NULL;
    } else if (iter->prev->removed) {
        return nat_hash_iter_prev(env, hash, iter->prev);
    } else {
        return iter->prev;
    }
}

NatHashIter *nat_hash_iter_next(NatEnv *env, NatObject *hash, NatHashIter *iter) {
    if (iter->next == NULL || (!iter->removed && iter->next == hash->key_list)) {
        // finished
        hash->hash_is_iterating = false;
        return NULL;
    } else if (iter->next->removed) {
        return nat_hash_iter_next(env, hash, iter->next);
    } else {
        return iter->next;
    }
}

NatObject *nat_hash(NatEnv *env) {
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Hash", true), NAT_VALUE_HASH);
    obj->key_list = NULL;
    obj->hash_default_value = NAT_NIL;
    obj->hash_default_block = NULL;
    hashmap_init(&obj->hashmap, nat_hashmap_hash, nat_hashmap_compare, 256);
    return obj;
}

NatObject *nat_hash_get(NatEnv *env, NatObject *hash, NatObject *key) {
    assert(NAT_TYPE(hash) == NAT_VALUE_HASH);
    NatHashKey key_container;
    key_container.key = key;
    key_container.env = *env;
    key_container.env.caller = NULL;
    NatHashVal *container = hashmap_get(&hash->hashmap, &key_container);
    NatObject *val = container ? container->val : NULL;
    return val;
}

NatObject *nat_hash_get_default(NatEnv *env, NatObject *hash, NatObject *key) {
    if (hash->hash_default_block) {
        NatObject *args[2] = { hash, key };
        return NAT_RUN_BLOCK_WITHOUT_BREAK(env, hash->hash_default_block, 2, args, NULL);
    } else {
        return hash->hash_default_value;
    }
}

void nat_hash_put(NatEnv *env, NatObject *hash, NatObject *key, NatObject *val) {
    assert(NAT_TYPE(hash) == NAT_VALUE_HASH);
    NatHashKey key_container;
    key_container.key = key;
    key_container.env = *env;
    key_container.env.caller = NULL;
    NatHashVal *container = hashmap_get(&hash->hashmap, &key_container);
    if (container) {
        container->key->val = val;
        container->val = val;
    } else {
        if (hash->hash_is_iterating) {
            NAT_RAISE(env, "RuntimeError", "can't add a new key into hash during iteration");
        }
        container = malloc(sizeof(NatHashVal));
        container->key = nat_hash_key_list_append(env, hash, key, val);
        container->val = val;
        hashmap_put(&hash->hashmap, container->key, container);
    }
}

NatObject *nat_hash_delete(NatEnv *env, NatObject *hash, NatObject *key) {
    assert(hash->type == NAT_VALUE_HASH);
    NatHashKey key_container;
    key_container.key = key;
    key_container.env = *env;
    key_container.env.caller = NULL;
    NatHashVal *container = hashmap_remove(&hash->hashmap, &key_container);
    if (container) {
        nat_hash_key_list_remove_node(hash, container->key);
        NatObject *val = container->val;
        free(container);
        return val;
    } else {
        return NULL;
    }
}

NatObject *nat_regexp(NatEnv *env, char *pattern) {
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
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Regexp", true), NAT_VALUE_REGEXP);
    obj->regexp = regexp;
    obj->regexp_str = heap_string(pattern);
    return obj;
}

NatObject *nat_matchdata(NatEnv *env, OnigRegion *region, NatObject *str_obj) {
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "MatchData", true), NAT_VALUE_MATCHDATA);
    obj->matchdata_region = region;
    assert(NAT_TYPE(str_obj) == NAT_VALUE_STRING);
    obj->matchdata_str = heap_string(str_obj->str);
    return obj;
}

NatObject *nat_last_match(NatEnv *env) {
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

static NatMethod *nat_method_from_fn(NatObject *(*fn)(NatEnv *, NatObject *, ssize_t, NatObject **, NatBlock *block)) {
    NatMethod *method = malloc(sizeof(NatMethod));
    method->fn = fn;
    method->env.global_env = NULL;
    method->undefined = fn ? false : true;
    return method;
}

static NatMethod *nat_method_from_block(NatBlock *block) {
    NatMethod *method = malloc(sizeof(NatMethod));
    method->fn = block->fn;
    method->env = block->env;
    method->env.caller = NULL;
    method->undefined = false;
    return method;
}

void nat_define_method(NatEnv *env, NatObject *obj, char *name, NatObject *(*fn)(NatEnv *, NatObject *, ssize_t, NatObject **, NatBlock *block)) {
    NatMethod *method = nat_method_from_fn(fn);
    if (nat_is_main_object(obj)) {
        free(hashmap_remove(&NAT_OBJ_CLASS(obj)->methods, name));
        hashmap_put(&NAT_OBJ_CLASS(obj)->methods, name, method);
    } else {
        free(hashmap_remove(&obj->methods, name));
        hashmap_put(&obj->methods, name, method);
    }
}

void nat_define_method_with_block(NatEnv *env, NatObject *obj, char *name, NatBlock *block) {
    NatMethod *method = nat_method_from_block(block);
    if (nat_is_main_object(obj)) {
        free(hashmap_remove(&NAT_OBJ_CLASS(obj)->methods, name));
        hashmap_put(&NAT_OBJ_CLASS(obj)->methods, name, method);
    } else {
        free(hashmap_remove(&obj->methods, name));
        hashmap_put(&obj->methods, name, method);
    }
}

void nat_define_singleton_method(NatEnv *env, NatObject *obj, char *name, NatObject *(*fn)(NatEnv *, NatObject *, ssize_t, NatObject **, NatBlock *block)) {
    NatMethod *method = nat_method_from_fn(fn);
    NatObject *klass = nat_singleton_class(env, obj);
    free(hashmap_remove(&klass->methods, name));
    hashmap_put(&klass->methods, name, method);
}

void nat_define_singleton_method_with_block(NatEnv *env, NatObject *obj, char *name, NatBlock *block) {
    NatMethod *method = nat_method_from_block(block);
    NatObject *klass = nat_singleton_class(env, obj);
    free(hashmap_remove(&klass->methods, name));
    hashmap_put(&klass->methods, name, method);
}

void nat_undefine_method(NatEnv *env, NatObject *obj, char *name) {
    nat_define_method(env, obj, name, NULL);
}

void nat_undefine_singleton_method(NatEnv *env, NatObject *obj, char *name) {
    nat_define_singleton_method(env, obj, name, NULL);
}

NatObject *nat_class_ancestors(NatEnv *env, NatObject *klass) {
    assert(NAT_TYPE(klass) == NAT_VALUE_CLASS || NAT_TYPE(klass) == NAT_VALUE_MODULE);
    NatObject *ancestors = nat_array(env);
    do {
        if (klass->included_modules_count == 0) {
            // note: if there are included modules, then they will include this klass
            nat_array_push(env, ancestors, klass);
        }
        for (ssize_t i = 0; i < klass->included_modules_count; i++) {
            nat_array_push(env, ancestors, klass->included_modules[i]);
        }
        klass = klass->superclass;
    } while (klass);
    return ancestors;
}

bool nat_is_a(NatEnv *env, NatObject *obj, NatObject *klass_or_module) {
    if (obj == klass_or_module) {
        return true;
    } else {
        NatObject *ancestors = nat_class_ancestors(env, NAT_OBJ_CLASS(obj));
        for (ssize_t i = 0; i < nat_vector_size(&ancestors->ary); i++) {
            if (klass_or_module == nat_vector_get(&ancestors->ary, i)) {
                return true;
            }
        }
        return false;
    }
}

char *nat_defined(NatEnv *env, NatObject *receiver, char *name) {
    NatObject *obj;
    if (nat_is_constant_name(name)) {
        obj = nat_const_get_or_null(env, receiver, name, false, false);
        if (obj) return "constant";
    } else if (nat_is_global_name(name)) {
        obj = nat_global_get(env, name);
        if (obj != NAT_NIL) return "global-variable";
    } else if (nat_respond_to(env, receiver, name)) {
        return "method";
    }
    return NULL;
}

NatObject *nat_defined_obj(NatEnv *env, NatObject *receiver, char *name) {
    char *result = nat_defined(env, receiver, name);
    if (result) {
        return nat_string(env, result);
    } else {
        return NAT_NIL;
    }
}

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, ssize_t argc, NatObject **args, NatBlock *block) {
    assert(receiver);
    NatObject *klass;
    if (NAT_TYPE(receiver) == NAT_VALUE_INTEGER) {
        klass = NAT_INTEGER;
    } else {
        klass = receiver->singleton_class;
        if (klass) {
            NatObject *matching_class_or_module;
            NatMethod *method = nat_find_method(klass, sym, &matching_class_or_module);
            if (method) {
#ifdef NAT_DEBUG_METHOD_RESOLUTION
                if (strcmp(sym, "inspect") != 0) {
                    if (method->undefined) {
                        fprintf(stderr, "Method %s found on %s and is marked undefined\n", sym, matching_class_or_module->class_name);
                    } else if (matching_class_or_module == klass) {
                        fprintf(stderr, "Method %s found on the singleton klass of %s\n", sym, nat_send(env, receiver, "inspect", 0, NULL, NULL)->str);
                    } else {
                        fprintf(stderr, "Method %s found on %s, which is an ancestor of the singleton klass of %s\n", sym, matching_class_or_module->class_name, nat_send(env, receiver, "inspect", 0, NULL, NULL)->str);
                    }
                }
#endif
                if (method->undefined) {
                    NAT_RAISE(env, "NoMethodError", "undefined method `%s' for %s:Class", sym, receiver->class_name);
                }
                return nat_call_method_on_class(env, klass, NAT_OBJ_CLASS(receiver), sym, receiver, argc, args, block);
            }
        }
        klass = NAT_OBJ_CLASS(receiver);
    }
#ifdef NAT_DEBUG_METHOD_RESOLUTION
    if (strcmp(sym, "inspect") != 0) {
        fprintf(stderr, "Looking for method %s in the klass hierarchy of %s\n", sym, nat_send(env, receiver, "inspect", 0, NULL, NULL)->str);
    }
#endif
    return nat_call_method_on_class(env, klass, klass, sym, receiver, argc, args, block);
}

// supply an empty array and it will be populated with the method names as symbols
void nat_methods(NatEnv *env, NatObject *array, NatObject *klass) {
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(&klass->methods); iter; iter = hashmap_iter_next(&klass->methods, iter)) {
        char *name = (char *)hashmap_iter_get_key(iter);
        nat_array_push(env, array, nat_symbol(env, name));
    }
    for (ssize_t i = 0; i < klass->included_modules_count; i++) {
        NatObject *module = klass->included_modules[i];
        for (iter = hashmap_iter(&module->methods); iter; iter = hashmap_iter_next(&module->methods, iter)) {
            char *name = (char *)hashmap_iter_get_key(iter);
            nat_array_push(env, array, nat_symbol(env, name));
        }
    }
    if (klass->superclass) {
        return nat_methods(env, array, klass->superclass);
    }
}

// returns the method and sets matching_class_or_module to where the method was found
NatMethod *nat_find_method(NatObject *klass, char *method_name, NatObject **matching_class_or_module) {
    assert(NAT_TYPE(klass) == NAT_VALUE_CLASS);

    NatMethod *method;
    if (klass->included_modules_count == 0) {
        // no included modules, just search the class/module
        // note: if there are included modules, then the module chain will include this class/module
        method = hashmap_get(&klass->methods, method_name);
        if (method) {
            *matching_class_or_module = klass;
            return method;
        }
    }

    for (ssize_t i = 0; i < klass->included_modules_count; i++) {
        NatObject *module = klass->included_modules[i];
        method = hashmap_get(&module->methods, method_name);
        if (method) {
            *matching_class_or_module = module;
            return method;
        }
    }

    if (klass->superclass) {
        return nat_find_method(klass->superclass, method_name, matching_class_or_module);
    } else {
        return NULL;
    }
}

NatMethod *nat_find_method_without_undefined(NatObject *klass, char *method_name, NatObject **matching_class_or_module) {
    NatMethod *method = nat_find_method(klass, method_name, matching_class_or_module);
    if (method && method->undefined) {
        return NULL;
    } else {
        return method;
    }
}

NatObject *nat_call_method_on_class(NatEnv *env, NatObject *klass, NatObject *instance_class, char *method_name, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    assert(klass != NULL);
    assert(NAT_TYPE(klass) == NAT_VALUE_CLASS);

    NatObject *matching_class_or_module;
    NatMethod *method = nat_find_method(klass, method_name, &matching_class_or_module);
    if (method && !method->undefined) {
#ifdef NAT_DEBUG_METHOD_RESOLUTION
        if (strcmp(method_name, "inspect") != 0) {
            fprintf(stderr, "Calling method %s from %s\n", method_name, matching_class_or_module->class_name);
        }
#endif
        NatEnv *closure_env;
        if (NAT_OBJ_HAS_ENV(method)) {
            closure_env = &method->env;
        } else {
            closure_env = &matching_class_or_module->env;
        }
        NatEnv e;
        nat_build_block_env(&e, closure_env, env);
        e.file = env->file;
        e.line = env->line;
        e.method_name = method_name;
        e.block = block;
        return method->fn(&e, self, argc, args, block);
    } else {
        NAT_RAISE(env, "NoMethodError", "undefined method `%s' for %v", method_name, instance_class);
    }
}

NatObject *nat_call_begin(NatEnv *env, NatObject *self, NatObject *(*block_fn)(NatEnv *, NatObject *)) {
    NatEnv e;
    nat_build_block_env(&e, env, env);
    return block_fn(&e, self);
}

bool nat_respond_to(NatEnv *env, NatObject *obj, char *name) {
    NatObject *matching_class_or_module;
    if (NAT_TYPE(obj) == NAT_VALUE_INTEGER) {
        NatObject *klass = NAT_INTEGER;
        if (nat_find_method_without_undefined(klass, name, &matching_class_or_module)) {
            return true;
        } else {
            return false;
        }
    } else if (obj->singleton_class && nat_find_method_without_undefined(obj->singleton_class, name, &matching_class_or_module)) {
        return true;
    } else if (nat_find_method_without_undefined(NAT_OBJ_CLASS(obj), name, &matching_class_or_module)) {
        return true;
    } else {
        return false;
    }
}

NatBlock *nat_block(NatEnv *env, NatObject *self, NatObject *(*fn)(NatEnv *, NatObject *, ssize_t, NatObject **, NatBlock *)) {
    NatBlock *block = malloc(sizeof(NatBlock));
    block->env = *env;
    block->env.caller = NULL;
    block->self = self;
    block->fn = fn;
    return block;
}

NatObject *_nat_run_block_internal(NatEnv *env, NatBlock *the_block, ssize_t argc, NatObject **args, NatBlock *block) {
    if (!the_block) {
        abort();
        NAT_RAISE(env, "LocalJumpError", "no block given");
    }
    NatEnv e;
    nat_build_block_env(&e, &the_block->env, env);
    return the_block->fn(&e, the_block->self, argc, args, block);
}

NatObject *nat_proc(NatEnv *env, NatBlock *block) {
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Proc", true), NAT_VALUE_PROC);
    obj->block = block;
    return obj;
}

NatObject *nat_to_proc(NatEnv *env, NatObject *obj) {
    if (NAT_TYPE(obj) == NAT_VALUE_PROC) {
        return obj;
    } else if (nat_respond_to(env, obj, "to_proc")) {
        return nat_send(env, obj, "to_proc", 0, NULL, NULL);
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Proc)", NAT_OBJ_CLASS(obj)->class_name);
    }
}

NatObject *nat_lambda(NatEnv *env, NatBlock *block) {
    NatObject *lambda = nat_proc(env, block);
    lambda->lambda = true;
    return lambda;
}

NatObject *nat_string_n(NatEnv *env, char *str, ssize_t len) {
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "String", true), NAT_VALUE_STRING);
    char *copy = malloc(len + 1);
    memcpy(copy, str, len);
    obj->str = copy;
    obj->str[len] = 0;
    obj->str_len = len;
    obj->str_cap = len;
    obj->encoding = NAT_ENCODING_UTF_8; // TODO: inherit from encoding of file?
    return obj;
}

NatObject *nat_string(NatEnv *env, char *str) {
    ssize_t len = strlen(str);
    return nat_string_n(env, str, len);
}

void nat_grow_string(NatEnv *env, NatObject *obj, ssize_t capacity) {
    ssize_t len = strlen(obj->str);
    assert(capacity >= len);
    obj->str = realloc(obj->str, capacity + 1);
    obj->str_cap = capacity;
}

void nat_grow_string_at_least(NatEnv *env, NatObject *obj, ssize_t min_capacity) {
    ssize_t capacity = obj->str_cap;
    if (capacity >= min_capacity)
        return;
    if (capacity > 0 && min_capacity <= capacity * NAT_STRING_GROW_FACTOR) {
        nat_grow_string(env, obj, capacity * NAT_STRING_GROW_FACTOR);
    } else {
        nat_grow_string(env, obj, min_capacity);
    }
}

void nat_string_append(NatEnv *env, NatObject *str, char *s) {
    ssize_t new_len = strlen(s);
    if (new_len == 0) return;
    ssize_t total_len = str->str_len + new_len;
    nat_grow_string_at_least(env, str, total_len);
    strcat(str->str, s);
    str->str_len = total_len;
}

void nat_string_append_char(NatEnv *env, NatObject *str, char c) {
    ssize_t total_len = str->str_len + 1;
    nat_grow_string_at_least(env, str, total_len);
    str->str[total_len - 1] = c;
    str->str[total_len] = 0;
    str->str_len = total_len;
}

void nat_string_append_nat_string(NatEnv *env, NatObject *str, NatObject *str2) {
    if (str2->str_len == 0) return;
    ssize_t total_len = str->str_len + str2->str_len;
    nat_grow_string_at_least(env, str, total_len);
    strncat(str->str, str2->str, str2->str_len);
    str->str_len = total_len;
}

#define NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, message_format, ...)                              \
    {                                                                                                         \
        NatObject *Encoding = nat_const_get(env, NAT_OBJECT, "Encoding", true);                               \
        NatObject *InvalidByteSequenceError = nat_const_get(env, Encoding, "InvalidByteSequenceError", true); \
        nat_raise(env, InvalidByteSequenceError, message_format, ##__VA_ARGS__);                              \
    }

NatObject *nat_string_chars(NatEnv *env, NatObject *str) {
    NatObject *ary = nat_array(env);
    NatObject *c;
    char buffer[5];
    switch (str->encoding) {
    case NAT_ENCODING_UTF_8:
        for (ssize_t i = 0; i < str->str_len; i++) {
            buffer[0] = str->str[i];
            if (((unsigned char)buffer[0] >> 3) == 30) { // 11110xxx, 4 bytes
                if (i + 3 >= str->str_len) abort(); //NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, nat_send(env, str, "inspect", 0, NULL, NULL));
                buffer[1] = str->str[++i];
                buffer[2] = str->str[++i];
                buffer[3] = str->str[++i];
                buffer[4] = 0;
            } else if (((unsigned char)buffer[0] >> 4) == 14) { // 1110xxxx, 3 bytes
                if (i + 2 >= str->str_len) abort(); //NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, nat_send(env, str, "inspect", 0, NULL, NULL));
                buffer[1] = str->str[++i];
                buffer[2] = str->str[++i];
                buffer[3] = 0;
            } else if (((unsigned char)buffer[0] >> 5) == 6) { // 110xxxxx, 2 bytes
                if (i + 1 >= str->str_len) abort(); //NAT_RAISE_ENCODING_INVALID_BYTE_SEQUENCE_ERROR(env, "invalid byte sequence at index %i in string %S (string not long enough)", i, nat_send(env, str, "inspect", 0, NULL, NULL));
                buffer[1] = str->str[++i];
                buffer[2] = 0;
            } else {
                buffer[1] = 0;
            }
            c = nat_string(env, buffer);
            c->encoding = NAT_ENCODING_UTF_8;
            nat_array_push(env, ary, c);
        }
        break;
    case NAT_ENCODING_ASCII_8BIT:
        for (ssize_t i = 0; i < str->str_len; i++) {
            buffer[0] = str->str[i];
            buffer[1] = 0;
            c = nat_string(env, buffer);
            c->encoding = NAT_ENCODING_ASCII_8BIT;
            nat_array_push(env, ary, c);
        }
        break;
    }
    return ary;
}

NatObject *nat_sprintf(NatEnv *env, char *format, ...) {
    va_list args;
    va_start(args, format);
    NatObject *out = nat_vsprintf(env, format, args);
    va_end(args);
    return out;
}

NatObject *nat_vsprintf(NatEnv *env, char *format, va_list args) {
    NatObject *out = nat_string(env, "");
    ssize_t len = strlen(format);
    NatObject *inspected;
    char buf[NAT_INT_64_MAX_BUF_LEN];
    for (ssize_t i = 0; i < len; i++) {
        char c = format[i];
        if (c == '%') {
            char c2 = format[++i];
            switch (c2) {
            case 's':
                nat_string_append(env, out, va_arg(args, char *));
                break;
            case 'S':
                nat_string_append_nat_string(env, out, va_arg(args, NatObject *));
                break;
            case 'i':
            case 'd':
                int_to_string(va_arg(args, int64_t), buf);
                nat_string_append(env, out, buf);
                break;
            case 'x':
                int_to_hex_string(va_arg(args, int64_t), buf, false);
                nat_string_append(env, out, buf);
                break;
            case 'X':
                int_to_hex_string(va_arg(args, int64_t), buf, true);
                nat_string_append(env, out, buf);
                break;
            case 'v':
                inspected = nat_send(env, va_arg(args, NatObject *), "inspect", 0, NULL, NULL);
                assert(NAT_TYPE(inspected) == NAT_VALUE_STRING);
                nat_string_append(env, out, inspected->str);
                break;
            case '%':
                nat_string_append_char(env, out, '%');
                break;
            default:
                fprintf(stderr, "Unknown format specifier: %%%c", c2);
                abort();
            }
        } else {
            nat_string_append_char(env, out, c);
        }
    }
    return out;
}

NatObject *nat_range(NatEnv *env, NatObject *begin, NatObject *end, bool exclude_end) {
    NatObject *obj = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Range", true), NAT_VALUE_RANGE);
    obj->range_begin = begin;
    obj->range_end = end;
    obj->range_exclude_end = exclude_end;
    return obj;
}

NatObject *nat_dup(NatEnv *env, NatObject *obj) {
    NatObject *copy = NULL;
    switch (NAT_TYPE(obj)) {
    case NAT_VALUE_ARRAY:
        copy = nat_array(env);
        for (ssize_t i = 0; i < nat_vector_size(&obj->ary); i++) {
            nat_array_push(env, copy, nat_vector_get(&obj->ary, i));
        }
        return copy;
    case NAT_VALUE_STRING:
        return nat_string(env, obj->str);
    case NAT_VALUE_SYMBOL:
        return nat_symbol(env, obj->symbol);
    case NAT_VALUE_FALSE:
    case NAT_VALUE_NIL:
    case NAT_VALUE_TRUE:
        return obj;
    default:
        fprintf(stderr, "I don't know how to dup this kind of object yet %d.\n", obj->type);
        abort();
    }
}

NatObject *nat_not(NatEnv *env, NatObject *val) {
    if (nat_truthy(val)) {
        return NAT_FALSE;
    } else {
        return NAT_TRUE;
    }
}

void nat_alias(NatEnv *env, NatObject *self, char *new_name, char *old_name) {
    if (NAT_TYPE(self) == NAT_VALUE_INTEGER || NAT_TYPE(self) == NAT_VALUE_SYMBOL) {
        NAT_RAISE(env, "TypeError", "no klass to make alias");
    }
    if (nat_is_main_object(self)) {
        self = NAT_OBJ_CLASS(self);
    }
    NatObject *klass = self;
    if (NAT_TYPE(self) != NAT_VALUE_CLASS && NAT_TYPE(self) != NAT_VALUE_MODULE) {
        klass = nat_singleton_class(env, self);
    }
    NatObject *matching_class_or_module;
    NatMethod *method = nat_find_method(klass, old_name, &matching_class_or_module);
    if (!method) {
        NAT_RAISE(env, "NameError", "undefined method `%s' for `%v'", old_name, klass);
    }
    free(hashmap_remove(&klass->methods, new_name));
    NatMethod *method_copy = malloc(sizeof(NatMethod));
    memcpy(method_copy, method, sizeof(NatMethod));
    hashmap_put(&klass->methods, new_name, method_copy);
}

void nat_run_at_exit_handlers(NatEnv *env) {
    NatObject *at_exit_handlers = nat_global_get(env, "$NAT_at_exit_handlers");
    assert(at_exit_handlers);
    for (int i = nat_vector_size(&at_exit_handlers->ary) - 1; i >= 0; i--) {
        NatObject *proc = nat_vector_get(&at_exit_handlers->ary, i);
        assert(proc);
        assert(NAT_TYPE(proc) == NAT_VALUE_PROC);
        NAT_RUN_BLOCK_WITHOUT_BREAK(env, proc->block, 0, NULL, NULL);
    }
}

void nat_print_exception_with_backtrace(NatEnv *env, NatObject *exception) {
    assert(NAT_TYPE(exception) == NAT_VALUE_EXCEPTION);
    NatObject *nat_stderr = nat_global_get(env, "$stderr");
    int fd = nat_stderr->fileno;
    if (nat_vector_size(&exception->backtrace->ary) > 0) {
        dprintf(fd, "Traceback (most recent call last):\n");
        for (int i = nat_vector_size(&exception->backtrace->ary) - 1; i > 0; i--) {
            NatObject *line = nat_vector_get(&exception->backtrace->ary, i);
            assert(NAT_TYPE(line) == NAT_VALUE_STRING);
            dprintf(fd, "        %d: from %s\n", i, line->str);
        }
        NatObject *line = nat_vector_get(&exception->backtrace->ary, 0);
        dprintf(fd, "%s: ", line->str);
    }
    dprintf(fd, "%s (%s)\n", exception->message, NAT_OBJ_CLASS(exception)->class_name);
}

void nat_handle_top_level_exception(NatEnv *env, bool run_exit_handlers) {
    NatObject *exception = env->exception;
    assert(exception);
    assert(NAT_TYPE(exception) == NAT_VALUE_EXCEPTION);
    env->rescue = false;
    if (nat_is_a(env, exception, nat_const_get(env, NAT_OBJECT, "SystemExit", true))) {
        NatObject *status_obj = nat_ivar_get(env, exception, "@status");
        if (run_exit_handlers) nat_run_at_exit_handlers(env);
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
        nat_print_exception_with_backtrace(env, exception);
    }
}

void nat_object_pointer_id(NatObject *obj, char *buf) {
    snprintf(buf, NAT_OBJECT_POINTER_BUF_LENGTH, "%p", obj);
}

int64_t nat_object_id(NatEnv *env, NatObject *obj) {
    if (NAT_TYPE(obj) == NAT_VALUE_INTEGER) {
        return (int64_t)obj;
    } else {
        return (int64_t)obj / 2;
    }
}

NatObject *nat_convert_to_real_object(NatEnv *env, NatObject *obj) {
    if (((int64_t)obj & 1)) {
        NatObject *real_obj = nat_alloc(env, NAT_INTEGER, NAT_VALUE_INTEGER);
        real_obj->integer = NAT_INT_VALUE(obj);
        return real_obj;
    } else {
        return obj;
    }
}

int nat_quicksort_partition(NatEnv *env, NatObject *ary[], int start, int end) {
    NatObject *pivot = ary[end];
    int pIndex = start;
    NatObject *temp;

    for (int i = start; i < end; i++) {
        NatObject *compare = nat_send(env, ary[i], "<=>", 1, &pivot, NULL);
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

void nat_quicksort(NatEnv *env, NatObject *ary[], int start, int end) {
    if (start < end) {
        int pIndex = nat_quicksort_partition(env, ary, start, end);
        nat_quicksort(env, ary, start, pIndex - 1);
        nat_quicksort(env, ary, pIndex + 1, end);
    }
}

NatObject *nat_to_ary(NatEnv *env, NatObject *obj, bool raise_for_non_array) {
    if (NAT_TYPE(obj) == NAT_VALUE_ARRAY) {
        return obj;
    } else if (nat_respond_to(env, obj, "to_ary")) {
        NatObject *ary = nat_send(env, obj, "to_ary", 0, NULL, NULL);
        if (NAT_TYPE(ary) == NAT_VALUE_ARRAY) {
            return ary;
        } else if (NAT_TYPE(ary) == NAT_VALUE_NIL || !raise_for_non_array) {
            ary = nat_array(env);
            nat_array_push(env, ary, obj);
            return ary;
        } else {
            char *class_name = NAT_OBJ_CLASS(obj)->class_name;
            NAT_RAISE(env, "TypeError", "can't convert %s to Array (%s#to_ary gives %s)", class_name, class_name, NAT_OBJ_CLASS(ary)->class_name);
        }
    } else {
        NatObject *ary = nat_array(env);
        nat_array_push(env, ary, obj);
        return ary;
    }
}

static NatObject *nat_splat_value(NatEnv *env, NatObject *value, ssize_t index, ssize_t offset_from_end) {
    NatObject *splat = nat_array(env);
    if (NAT_TYPE(value) == NAT_VALUE_ARRAY && index < nat_vector_size(&value->ary) - offset_from_end) {
        for (ssize_t s = index; s < nat_vector_size(&value->ary) - offset_from_end; s++) {
            nat_array_push(env, splat, nat_vector_get(&value->ary, s));
        }
    }
    return splat;
}

NatObject *nat_arg_value_by_path(NatEnv *env, NatObject *value, NatObject *default_value, bool splat, int total_count, int default_count, bool defaults_on_right, int offset_from_end, ssize_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    bool has_default = default_value != NAT_NIL;
    bool defaults_on_left = !defaults_on_right;
    int required_count = total_count - default_count;
    NatObject *return_value = value;
    for (ssize_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);

        if (splat && i == path_size - 1) {
            return nat_splat_value(env, return_value, index, offset_from_end);
        } else {
            if (NAT_TYPE(return_value) == NAT_VALUE_ARRAY) {

                int64_t ary_len = (int64_t)nat_vector_size(&return_value->ary);

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
                    return_value = nat_vector_get(&return_value->ary, index);

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

NatObject *nat_array_value_by_path(NatEnv *env, NatObject *value, NatObject *default_value, bool splat, int offset_from_end, ssize_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    NatObject *return_value = value;
    for (ssize_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);
        if (splat && i == path_size - 1) {
            return nat_splat_value(env, return_value, index, offset_from_end);
        } else {
            if (NAT_TYPE(return_value) == NAT_VALUE_ARRAY) {

                int64_t ary_len = (int64_t)nat_vector_size(&return_value->ary);

                if (index < 0) {
                    // negative offset index should go from the right
                    index = ary_len + index;
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = nat_vector_get(&return_value->ary, index);

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

NatObject *nat_kwarg_value_by_name(NatEnv *env, NatObject *args, char *name, NatObject *default_value) {
    assert(NAT_TYPE(args) == NAT_VALUE_ARRAY);
    NatObject *hash;
    if (nat_vector_size(&args->ary) == 0) {
        hash = nat_hash(env);
    } else {
        hash = nat_vector_get(&args->ary, nat_vector_size(&args->ary) - 1);
        if (NAT_TYPE(hash) != NAT_VALUE_HASH) {
            hash = nat_hash(env);
        }
    }
    NatObject *value = nat_hash_get(env, hash, nat_symbol(env, name));
    if (!value) {
        if (default_value) {
            return default_value;
        } else {
            NAT_RAISE(env, "ArgumentError", "missing keyword: :%s", name);
        }
    }
    return value;
}

NatObject *nat_args_to_array(NatEnv *env, ssize_t argc, NatObject **args) {
    NatObject *ary = nat_array(env);
    for (ssize_t i = 0; i < argc; i++) {
        nat_array_push(env, ary, args[i]);
    }
    return ary;
}

// much like nat_args_to_array above, but when a block is given a single arg,
// and the block wants multiple args, call nat_to_ary on the first arg and return that
NatObject *nat_block_args_to_array(NatEnv *env, ssize_t signature_size, ssize_t argc, NatObject **args) {
    if (argc == 1 && signature_size > 1) {
        return nat_to_ary(env, args[0], true);
    }
    return nat_args_to_array(env, argc, args);
}

NatObject *nat_encoding(NatEnv *env, int num, NatObject *names) {
    NatObject *encoding = nat_alloc(env, nat_const_get(env, NAT_OBJECT, "Encoding", true), NAT_VALUE_ENCODING);
    encoding->encoding_num = num;
    encoding->encoding_names = names;
    nat_freeze_object(names);
    return encoding;
}

NatObject *nat_eval_class_or_module_body(NatEnv *env, NatObject *class_or_module, NatObject *(*fn)(NatEnv *, NatObject *)) {
    NatEnv body_env;
    nat_build_env(&body_env, env);
    body_env.caller = env;
    NatObject *result = fn(&body_env, class_or_module);
    body_env.caller = NULL;
    return result;
}

void nat_arg_spread(NatEnv *env, ssize_t argc, NatObject **args, char *arrangement, ...) {
    va_list va_args;
    va_start(va_args, arrangement);
    ssize_t len = strlen(arrangement);
    ssize_t arg_index = 0;
    NatObject *obj;
    bool *bool_ptr;
    int *int_ptr;
    char **str_ptr;
    void **void_ptr;
    NatObject **obj_ptr;
    for (ssize_t i = 0; i < len; i++) {
        char c = arrangement[i];
        switch (c) {
        case 'o':
            obj_ptr = va_arg(va_args, NatObject **);
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
            *bool_ptr = nat_truthy(obj);
            break;
        case 'v':
            void_ptr = va_arg(va_args, void **);
            if (arg_index >= argc) NAT_RAISE(env, "ArgumentError", "wrong number of arguments (given %d, expected %d)", argc, arg_index + 1);
            obj = args[arg_index++];
            obj = nat_ivar_get(env, obj, "@_ptr");
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

NatObject *nat_void_ptr(NatEnv *env, void *ptr) {
    NatObject *obj = nat_alloc(env, NAT_OBJECT, NAT_VALUE_VOIDP);
    obj->void_ptr = ptr;
    return obj;
}
