#include "natalie.h"
#include <ctype.h>
#include <stdarg.h>

int is_constant_name(char *name) {
    return strlen(name) > 0 && isupper(name[0]);
}

int is_special_name(char *name) {
    return strcmp(name, "nil") == 0 || strcmp(name, "true") == 0 || strcmp(name, "false") == 0;
}

NatObject *env_get(NatEnv *env, char *key) {
    NatObject *val;
    int is_special = is_constant_name(key) || is_special_name(key);
    while (!(val = hashmap_get(&env->data, key)) && env->outer && (env->block || is_special)) {
        env = env->outer;
    }
    if (val) {
        return val;
    } else {
        return NULL;
    }
}

NatObject *env_set(NatEnv *env, char *key, NatObject *val) {
    if (env->block && env_get(env->outer, key)) {
        env = env->outer;
    }
    hashmap_remove(&env->data, key);
    hashmap_put(&env->data, key, val);
    return val;
}

NatEnv *build_env(NatEnv *outer) {
    NatEnv *env = malloc(sizeof(NatEnv));
    env->block = FALSE;
    env->outer = outer;
    env->jump_buf = NULL;
    env->caller = NULL;
    if (outer) {
        env->globals = outer->globals;
        env->symbols = outer->symbols;
        env->next_object_id = outer->next_object_id;
    } else {
        struct hashmap *global_table = malloc(sizeof(struct hashmap));
        hashmap_init(global_table, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(global_table, hashmap_alloc_key_string, NULL);
        env->globals = global_table;
        struct hashmap *symbol_table = malloc(sizeof(struct hashmap));
        hashmap_init(symbol_table, hashmap_hash_string, hashmap_compare_string, 100);
        hashmap_set_key_alloc_funcs(symbol_table, hashmap_alloc_key_string, NULL);
        env->symbols = symbol_table;
        env->next_object_id = malloc(sizeof(uint64_t));
        *env->next_object_id = 1;
    }
    hashmap_init(&env->data, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&env->data, hashmap_alloc_key_string, NULL);
    return env;
}

NatEnv *build_block_env(NatEnv *outer, NatEnv *calling_env) {
    NatEnv *env = build_env(outer);
    env->block = TRUE;
    env->caller = calling_env;
    return env;
}

NatObject* nat_raise(NatEnv *env, NatObject *klass, char *message_format, ...) {
    va_list args;
    va_start(args, message_format);
    NatObject *message = nat_vsprintf(env, message_format, args);
    va_end(args);
    NatObject *exception = nat_exception(env, klass, message->str);
    nat_raise_exception(env, exception);
    return exception;
}

NatObject* nat_raise_exception(NatEnv *env, NatObject *exception) {
    int counter = 0;
    while (!env->jump_buf) {
        assert(++counter < 10000); // serious problem

        if (env->caller) {
            env = env->caller;
        } else {
            env = env->outer;
        }
    }
    env->exception = exception;
    longjmp(*env->jump_buf, 1);
}

NatObject *ivar_get(NatEnv *env, NatObject *obj, char *name) {
    assert(strlen(name) > 0);
    if(name[0] != '@') {
        NAT_RAISE(env, env_get(env, "NameError"), "`%s' is not allowed as an instance variable name", name);
    }
    NatObject *val = hashmap_get(&obj->ivars, name);
    if (val) {
        return val;
    } else {
        return env_get(env, "nil");
    }
}

NatObject *ivar_set(NatEnv *env, NatObject *obj, char *name, NatObject *val) {
    assert(strlen(name) > 0);
    if(name[0] != '@') {
        NAT_RAISE(env, env_get(env, "NameError"), "`%s' is not allowed as an instance variable name", name);
    }
    hashmap_remove(&obj->ivars, name);
    hashmap_put(&obj->ivars, name, val);
    return val;
}

NatObject *global_get(NatEnv *env, char *name) {
    assert(strlen(name) > 0);
    if(name[0] != '$') {
        NAT_RAISE(env, env_get(env, "NameError"), "`%s' is not allowed as an global variable name", name);
    }
    NatObject *val = hashmap_get(env->globals, name);
    if (val) {
        return val;
    } else {
        return env_get(env, "nil");
    }
}

NatObject *global_set(NatEnv *env, char *name, NatObject *val) {
    assert(strlen(name) > 0);
    if(name[0] != '$') {
        NAT_RAISE(env, env_get(env, "NameError"), "`%s' is not allowed as an global variable name", name);
    }
    hashmap_remove(env->globals, name);
    hashmap_put(env->globals, name, val);
    return val;
}

#define TRUE 1
#define FALSE 0

int nat_truthy(NatObject *obj) {
    if (obj->type == NAT_VALUE_FALSE || obj->type == NAT_VALUE_NIL) {
        return FALSE;
    } else {
        return TRUE;
    }
}

char *heap_string(char *str) {
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

NatObject *nat_alloc(NatEnv *env) {
    NatObject *obj = malloc(sizeof(NatObject));
    obj->flags = 0;
    obj->type = NAT_VALUE_OTHER;
    obj->env = NULL;
    obj->included_modules_count = 0;
    obj->included_modules = NULL;
    obj->id = nat_next_object_id(env);
    obj->singleton_class = NULL;
    hashmap_init(&obj->ivars, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&obj->ivars, hashmap_alloc_key_string, NULL);
    return obj;
}

NatObject *nat_subclass(NatEnv *env, NatObject *superclass, char *name) {
    NatObject *val = nat_alloc(env);
    val->type = NAT_VALUE_CLASS;
    val->class = superclass->class;
    if (superclass->singleton_class) {
        val->singleton_class = nat_subclass(env, superclass->singleton_class, NULL);
    }
    val->class_name = name ? heap_string(name) : NULL;
    val->superclass = superclass;
    val->env = build_env(superclass->env);
    hashmap_init(&val->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->methods, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_module(NatEnv *env, char *name) {
    NatObject *val = nat_alloc(env);
    val->type = NAT_VALUE_MODULE;
    val->class = env_get(env, "Module");
    val->class_name = name ? heap_string(name) : NULL;
    val->env = build_env(env);
    hashmap_init(&val->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->methods, hashmap_alloc_key_string, NULL);
    return val;
}

void nat_class_include(NatObject *class, NatObject *module) {
    class->included_modules_count++;
    if (class->included_modules_count == 1) {
        class->included_modules = malloc(sizeof(NatObject*));
    } else {
        class->included_modules = realloc(class->included_modules, sizeof(NatObject*) * class->included_modules_count);
    }
    class->included_modules[class->included_modules_count - 1] = module;
}

NatObject *nat_new(NatEnv *env, NatObject *class, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *obj = nat_alloc(env);
    obj->class = class;
    while (1) {
        if (hashmap_get(&class->methods, "initialize")) {
            nat_call_method_on_class(env, class, class, "initialize", obj, argc, args, kwargs, block);
        }
        if (nat_is_top_class(class)) break;
        class = class->superclass;
    }
    return obj;
}

NatObject *nat_singleton_class(NatEnv *env, NatObject *obj) {
    if (!obj->singleton_class) {
        obj->singleton_class = nat_subclass(env, obj->class, NULL);
    }
    return obj->singleton_class;
}

NatObject *nat_integer(NatEnv *env, int64_t integer) {
    NatObject *obj = nat_new(env, env_get(env, "Integer"), 0, NULL, NULL, NULL);
    obj->type = NAT_VALUE_INTEGER;
    obj->integer = integer;
    return obj;
}

NatObject *nat_string(NatEnv *env, char *str) {
    NatObject *obj = nat_new(env, env_get(env, "String"), 0, NULL, NULL, NULL);
    obj->type = NAT_VALUE_STRING;
    size_t len = strlen(str);
    obj->str = heap_string(str);
    obj->str_len = len;
    obj->str_cap = len;
    return obj;
}

NatObject *nat_symbol(NatEnv *env, char *name) {
    NatObject *symbol = hashmap_get(env->symbols, name);
    if (symbol) {
        return symbol;
    } else {
        symbol = nat_new(env, env_get(env, "Symbol"), 0, NULL, NULL, NULL);
        symbol->type = NAT_VALUE_SYMBOL;
        symbol->symbol = name;
        hashmap_remove(env->symbols, name);
        hashmap_put(env->symbols, name, symbol);
        return symbol;
    }
}

NatObject *nat_exception(NatEnv *env, NatObject *klass, char *message) {
    NatObject *obj = nat_new(env, klass, 0, NULL, NULL, NULL);
    obj->type = NAT_VALUE_EXCEPTION;
    obj->message = message;
    return obj;
}

NatObject *nat_array(NatEnv *env) {
    NatObject *obj = nat_new(env, env_get(env, "Array"), 0, NULL, NULL, NULL);
    obj->type = NAT_VALUE_ARRAY;
    obj->ary = calloc(NAT_ARRAY_INIT_SIZE, sizeof(NatObject*));
    obj->str_len = 0;
    obj->str_cap = NAT_ARRAY_INIT_SIZE;
    return obj;
}

void nat_grow_array(NatObject *obj, size_t capacity) {
    obj->ary = realloc(obj->ary, sizeof(NatObject*) * capacity);
    obj->ary_cap = capacity;
}

void nat_grow_array_at_least(NatObject *obj, size_t min_capacity) {
    size_t capacity = obj->ary_cap;
    if (capacity >= min_capacity)
        return;
    if (capacity > 0 && min_capacity <= capacity * NAT_ARRAY_GROW_FACTOR) {
        nat_grow_array(obj, capacity * NAT_ARRAY_GROW_FACTOR);
    } else {
        nat_grow_array(obj, min_capacity);
    }
}

void nat_array_push(NatObject *array, NatObject *obj) {
    assert(array->type == NAT_VALUE_ARRAY);
    size_t capacity = array->ary_cap;
    size_t len = array->ary_len;
    if (len >= capacity) {
        nat_grow_array_at_least(array, len + 1);
    }
    array->ary_len++;
    array->ary[len] = obj;
}

void nat_array_push_splat(NatEnv *env, NatObject *array, NatObject *obj) {
    assert(array->type == NAT_VALUE_ARRAY);
    if (obj->type != NAT_VALUE_ARRAY && nat_respond_to(obj, "to_a")) {
        obj = nat_send(env, obj, "to_a", 0, NULL, NULL);
    }
    if (obj->type == NAT_VALUE_ARRAY) {
        for (size_t i=0; i<obj->ary_len; i++) {
            nat_array_push(array, obj->ary[i]);
        }
    } else {
        nat_array_push(array, obj);
    }
}

void nat_array_expand_with_nil(NatEnv *env, NatObject *array, size_t size) {
    assert(array->type == NAT_VALUE_ARRAY);
    NatObject *nil = env_get(env, "nil");
    for (size_t i=array->ary_len; i<size; i++) {
        nat_array_push(array, nil);
    }
}

#define INT_64_MAX_CHAR_LEN 21 // 1 for sign, 19 for max digits, and 1 for null terminator

char* int_to_string(int64_t num) {
    char* str;
    if (num == 0) {
        return heap_string("0");
    } else {
        str = malloc(INT_64_MAX_CHAR_LEN);
        snprintf(str, INT_64_MAX_CHAR_LEN, "%" PRId64, num);
        return str;
    }
}

void nat_define_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *block)) {
    NatMethod *method = malloc(sizeof(NatMethod));
    method->fn = fn;
    method->env = NULL;
    if (nat_is_main_object(obj)) {
        hashmap_remove(&obj->class->methods, name);
        hashmap_put(&obj->class->methods, name, method);
    } else {
        hashmap_remove(&obj->methods, name);
        hashmap_put(&obj->methods, name, method);
    }
}

void nat_define_method_with_block(NatObject *obj, char *name, NatBlock *block) {
    NatMethod *method = malloc(sizeof(NatMethod));
    method->fn = block->fn;
    method->env = block->env;
    if (nat_is_main_object(obj)) {
        hashmap_remove(&obj->class->methods, name);
        hashmap_put(&obj->class->methods, name, method);
    } else {
        hashmap_remove(&obj->methods, name);
        hashmap_put(&obj->methods, name, method);
    }
}

void nat_define_singleton_method(NatEnv *env, NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *block)) {
    NatMethod *method = malloc(sizeof(NatMethod));
    method->fn = fn;
    method->env = NULL;
    NatObject *klass = nat_singleton_class(env, obj);
    hashmap_remove(&klass->methods, name);
    hashmap_put(&klass->methods, name, method);
}

NatObject *nat_class_ancestors(NatEnv *env, NatObject *klass) {
    assert(klass->type == NAT_VALUE_CLASS || klass->type == NAT_VALUE_MODULE);
    NatObject *ancestors = nat_array(env);
    while (1) {
        nat_array_push(ancestors, klass);
        for (size_t i=0; i<klass->included_modules_count; i++) {
            nat_array_push(ancestors, klass->included_modules[i]);
        }
        if (!klass->superclass) break;
        klass = klass->superclass;
    }
    return ancestors;
}

int nat_is_a(NatEnv *env, NatObject *obj, NatObject *klass_or_module) {
    if (obj == klass_or_module) {
        return TRUE;
    } else {
        NatObject *ancestors = nat_class_ancestors(env, obj->class);
        for (size_t i=0; i<ancestors->ary_len; i++) {
            if (klass_or_module == ancestors->ary[i]) {
                return TRUE;
            }
        }
        return FALSE;
    }
}

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args, NatBlock *block) { // FIXME: kwargs
    assert(receiver);
    NatObject *klass = receiver->singleton_class;
    if (klass) {
        NatObject *matching_class_or_module;
        NatMethod *method = nat_find_method(klass, sym, &matching_class_or_module);
        if (method) {
#ifdef DEBUG_METHOD_RESOLUTION
            if (strcmp(sym, "inspect") != 0) {
                if (matching_class_or_module == klass) {
                    fprintf(stderr, "Method %s found on the singleton class of %s\n", sym, nat_send(env, receiver, "inspect", 0, NULL, NULL)->str);
                } else {
                    fprintf(stderr, "Method %s found on %s, which is an ancestor of the singleton class of %s\n", sym, matching_class_or_module->class_name, nat_send(env, receiver, "inspect", 0, NULL, NULL)->str);
                }
            }
#endif
            return nat_call_method_on_class(env, klass, receiver->class, sym, receiver, argc, args, NULL, block);
        }
    }
    klass = receiver->class;
#ifdef DEBUG_METHOD_RESOLUTION
    if (strcmp(sym, "inspect") != 0) {
        fprintf(stderr, "Looking for method %s in the class hierarchy of %s\n", sym, nat_send(env, receiver, "inspect", 0, NULL, NULL)->str);
    }
#endif
    return nat_call_method_on_class(env, klass, klass, sym, receiver, argc, args, NULL, block);
}

// supply an empty array and it will be populated with the method names as symbols
void nat_methods(NatEnv *env, NatObject *array, NatObject *klass) {
    struct hashmap_iter *iter;
    for (iter = hashmap_iter(&klass->methods); iter; iter = hashmap_iter_next(&klass->methods, iter))
    {
        char *name = (char *)hashmap_iter_get_key(iter);
        nat_array_push(array, nat_symbol(env, name));
    }
    for (size_t i=0; i<klass->included_modules_count; i++) {
        NatObject *module = klass->included_modules[i];
        for (iter = hashmap_iter(&module->methods); iter; iter = hashmap_iter_next(&module->methods, iter))
        {
            char *name = (char *)hashmap_iter_get_key(iter);
            nat_array_push(array, nat_symbol(env, name));
        }
    }
    if (klass->superclass) {
        return nat_methods(env, array, klass->superclass);
    }
}

// returns the method and sets matching_class_or_module to where the method was found
NatMethod *nat_find_method(NatObject *klass, char *method_name, NatObject **matching_class_or_module) {
    assert(klass->type == NAT_VALUE_CLASS);

    NatMethod *method = hashmap_get(&klass->methods, method_name);
    if (method) {
        *matching_class_or_module = klass;
        return method;
    }

    for (size_t i=0; i<klass->included_modules_count; i++) {
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

NatObject *nat_call_method_on_class(NatEnv *env, NatObject *class, NatObject *instance_class, char *method_name, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(class != NULL);
    assert(class->type == NAT_VALUE_CLASS);

    NatObject *matching_class_or_module;
    NatMethod *method = nat_find_method(class, method_name, &matching_class_or_module);
    if (method) {
#ifdef DEBUG_METHOD_RESOLUTION
        if (strcmp(method_name, "inspect") != 0) {
            fprintf(stderr, "Calling method %s from %s\n", method_name, matching_class_or_module->class_name);
        }
#endif
        NatEnv *closure_env;
        if (method->env) {
            closure_env = method->env;
        } else if (self->type == NAT_VALUE_CLASS) {
            // FIXME: not sure if this is proper, but it works
            closure_env = self->env;
        } else {
            closure_env = matching_class_or_module->env;
        }
        NatEnv *e = build_block_env(closure_env, env);
        return method->fn(e, self, argc, args, NULL, block);
    } else {
        NAT_RAISE(env, env_get(env, "NoMethodError"), "undefined method `%s' for %s", method_name, nat_send(env, instance_class, "inspect", 0, NULL, NULL)->str);
    }
}

NatObject *nat_lookup_or_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args, NatBlock *block) {
    if (argc > 0) {
        return nat_send(env, receiver, sym, argc, args, block);
    } else {
        NatObject *obj;
        if (receiver->env && is_constant_name(sym)) {
            obj = env_get(receiver->env, sym);
            if (obj) return obj;
        } else {
            obj = env_get(env, sym);
            if (obj) return obj;
        }
        return nat_send(env, receiver, sym, argc, args, block);
    }
}

NatObject *nat_lookup(NatEnv *env, char *name) {
    NatObject *obj = env_get(env, name);
    if (obj) {
        return obj;
    } else {
        NAT_RAISE(env, env_get(env, "NameError"), "undefined local variable or method `%s'", name);
    }
}

int nat_respond_to(NatObject *obj, char *name) {
    NatObject *matching_class_or_module;
    // FIXME: I don't think we need to check both singleton_class and class since singleton_class inherits from the class
    if (obj->singleton_class && nat_find_method(obj->singleton_class, name, &matching_class_or_module)) {
        return TRUE;
    } else if (nat_find_method(obj->class, name, &matching_class_or_module)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

NatBlock *nat_block(NatEnv *env, NatObject *self, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock*)) {
    NatBlock *block = malloc(sizeof(NatBlock));
    block->env = env;
    block->self = self;
    block->fn = fn;
    return block;
}

NatObject *nat_run_block(NatEnv *env, NatBlock *the_block, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatEnv *e = build_block_env(the_block->env, env);
    return the_block->fn(e, the_block->self, argc, args, kwargs, block);
}

NatObject *nat_proc(NatEnv *env, NatBlock *block) {
    NatObject *obj = nat_new(env, env_get(env, "Proc"), 0, NULL, NULL, NULL);
    obj->type = NAT_VALUE_PROC;
    obj->block = block;
    return obj;
}

NatObject *nat_lambda(NatEnv *env, NatBlock *block) {
    NatObject *lambda = nat_proc(env, block);
    lambda->lambda = TRUE;
    return lambda;
}

// "0x" + up to 16 hex chars + NULL terminator
#define NAT_OBJECT_POINTER_LENGTH 2 + 16 + 1

char *nat_object_pointer_id(NatObject *obj) {
    char *ptr = malloc(NAT_OBJECT_POINTER_LENGTH);
    snprintf(ptr, NAT_OBJECT_POINTER_LENGTH, "%p", obj);
    return ptr;
}

uint64_t nat_next_object_id(NatEnv *env) {
    return (*env->next_object_id)++;
}

void nat_grow_string(NatObject *obj, size_t capacity) {
    size_t len = strlen(obj->str);
    assert(capacity >= len);
    obj->str = realloc(obj->str, capacity + 1);
    obj->str_cap = capacity;
}

void nat_grow_string_at_least(NatObject *obj, size_t min_capacity) {
    size_t capacity = obj->str_cap;
    if (capacity >= min_capacity)
        return;
    if (capacity > 0 && min_capacity <= capacity * NAT_STRING_GROW_FACTOR) {
        nat_grow_string(obj, capacity * NAT_STRING_GROW_FACTOR);
    } else {
        nat_grow_string(obj, min_capacity);
    }
}

void nat_string_append(NatObject *str, char *s) {
    size_t new_len = strlen(s);
    if (new_len == 0) return;
    size_t total_len = str->str_len + new_len;
    nat_grow_string_at_least(str, total_len);
    strcat(str->str, s);
    str->str_len = total_len;
}

void nat_string_append_char(NatObject *str, char c) {
    size_t total_len = str->str_len + 1;
    nat_grow_string_at_least(str, total_len);
    str->str[total_len - 1] = c;
    str->str[total_len] = 0;
    str->str_len = total_len;
}

void nat_string_append_nat_string(NatObject *str, NatObject *str2) {
    if (str2->str_len == 0) return;
    size_t total_len = str->str_len + str2->str_len;
    nat_grow_string_at_least(str, total_len);
    strcat(str->str, str2->str);
    str->str_len = total_len;
}

NatObject* nat_sprintf(NatEnv *env, char *format, ...) {
    va_list args;
    va_start(args, format);
    NatObject *out = nat_vsprintf(env, format, args);
    va_end(args);
    return out;
}

NatObject* nat_vsprintf(NatEnv *env, char *format, va_list args) {
    NatObject *out = nat_string(env, "");
    char c, c2;
    size_t len = strlen(format);
    for (size_t i=0; i<len; i++) {
        c = format[i];
        if (c == '%') {
            c2 = format[++i];
            switch (c2) {
                case 's':
                    nat_string_append(out, va_arg(args, char*));
                    break;
                case 'i':
                case 'd':
                    nat_string_append(out, int_to_string(va_arg(args, int)));
                    break;
                /*
                   case 'S':
                   nat_string_append_nat_string(out, va_arg(args, NatObject*));
                   break;
                case 'z':
                    mal_string_append(out, pr_str(mal_number(va_arg(args, size_t)), 1));
                    break;
                case 'p':
                    mal_string_append(out, pr_str(mal_number((size_t)va_arg(args, NatObject*)), 1));
                    break;
                   */
                case '%':
                    nat_string_append_char(out, '%');
                    break;
                default:
                    fprintf(stderr, "Unknown format specifier: %%%c", c2);
                    abort();
            }
        } else {
            nat_string_append_char(out, c);
        }
    }
    return out;
}

NatObject *nat_dup(NatEnv *env, NatObject *obj) {
    NatObject *copy;
    switch (obj->type) {
        case NAT_VALUE_ARRAY:
            copy = nat_array(env);
            for (size_t i=0; i<obj->ary_len; i++) {
                nat_array_push(copy, obj->ary[i]);
            }
            return copy;
        case NAT_VALUE_STRING:
            return nat_string(env, obj->str);
        case NAT_VALUE_SYMBOL:
            return nat_symbol(env, obj->symbol);
        case NAT_VALUE_CLASS:
        case NAT_VALUE_EXCEPTION:
        case NAT_VALUE_FALSE:
        case NAT_VALUE_INTEGER:
        case NAT_VALUE_MODULE:
        case NAT_VALUE_NIL:
        case NAT_VALUE_OTHER:
        case NAT_VALUE_PROC:
        case NAT_VALUE_TRUE:
            memcpy(copy, obj, sizeof(NatObject));
            return copy;
        default:
            fprintf(stderr, "I don't know how to dup this kind of object.\n");
            abort();
    }
}

NatObject *nat_not(NatEnv *env, NatObject *val) {
    if (nat_truthy(val)) {
        return env_get(env, "false");
    } else {
        return env_get(env, "true");
    }
}

void nat_alias(NatEnv *env, NatObject *self, char *new_name, char *old_name) {
    if (self->type == NAT_VALUE_INTEGER || self->type == NAT_VALUE_SYMBOL) {
        NAT_RAISE(env, env_get(env, "TypeError"), "no class to make alias");
    }
    if (nat_is_main_object(self)) {
        self = self->class;
    }
    NatObject *klass = self;
    if (self->type != NAT_VALUE_CLASS && self->type != NAT_VALUE_MODULE) {
        klass = nat_singleton_class(env, self);
    }
    NatObject *matching_class_or_module;
    NatMethod *method = nat_find_method(klass, old_name, &matching_class_or_module);
    if (!method) {
        NAT_RAISE(env, env_get(env, "NameError"), "undefined method `%s' for class `%s'", old_name, nat_send(env, klass, "inspect", 0, NULL, NULL)->str);
    }
    hashmap_remove(&klass->methods, new_name);
    hashmap_put(&klass->methods, new_name, method);
}