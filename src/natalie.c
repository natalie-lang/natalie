#include "natalie.h"
#include <ctype.h>

NatEnv *env_find(NatEnv *env, char *key) {
    if (hashmap_get(&env->data, key)) {
        return env;
    } else if (env->outer) {
        return env_find(env->outer, key);
    } else {
        return NULL;
    }
}

NatObject *env_get(NatEnv *env, char *key) {
    if (strlen(key) > 0 && (isupper(key[0]) || strcmp(key, "nil") == 0 || strcmp(key, "true") == 0 || strcmp(key, "false") == 0)) {
        env = env_find(env, key);
        if (!env) {
            return NULL;
        }
    }
    NatObject *val = hashmap_get(&env->data, key);
    if (val) {
        return val;
    } else {
        return NULL;
    }
}

NatObject *env_set(NatEnv *env, char *key, NatObject *val) {
    hashmap_remove(&env->data, key);
    hashmap_put(&env->data, key, val);
    return val;
}

NatEnv *build_env(NatEnv *outer) {
    NatEnv *env = malloc(sizeof(NatEnv));
    env->outer = outer;
    if (outer) {
        env->symbols = outer->symbols;
        env->next_object_id = outer->next_object_id;
    } else {
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

NatObject *ivar_get(NatEnv *env, NatObject *obj, char *name) {
    NatObject *val = hashmap_get(&obj->ivars, name);
    if (val) {
        return val;
    } else {
        return env_get(env, "nil");
    }
}

void ivar_set(NatEnv *env, NatObject *obj, char *name, NatObject *val) {
    hashmap_remove(&obj->ivars, name);
    hashmap_put(&obj->ivars, name, val);
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
    NatObject *val = malloc(sizeof(NatObject));
    val->flags = 0;
    val->type = NAT_VALUE_OTHER;
    val->included_modules_count = 0;
    val->included_modules = NULL;
    val->id = nat_next_object_id(env);
    hashmap_init(&val->singleton_methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->singleton_methods, hashmap_alloc_key_string, NULL);
    hashmap_init(&val->ivars, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->ivars, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_subclass(NatEnv *env, NatObject *superclass, char *name) {
    NatObject *val = nat_alloc(env);
    val->type = NAT_VALUE_CLASS;
    val->class = superclass->class;
    val->class_name = name ? heap_string(name) : NULL;
    val->superclass = superclass;
    hashmap_init(&val->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->methods, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_module(NatEnv *env, char *name) {
    NatObject *val = nat_alloc(env);
    val->type = NAT_VALUE_MODULE;
    val->class = env_get(env, "Module");
    val->class_name = name ? heap_string(name) : NULL;
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
    if (hashmap_get(&class->methods, "initialize")) {
        nat_call_method_on_class(env, class, class, "initialize", obj, argc, args, kwargs, block);
    }
    return obj;
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
        hashmap_put(env->symbols, name, symbol);
        return symbol;
    }
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
    if (nat_is_main_object(obj)) {
        hashmap_put(&obj->class->methods, name, fn);
    } else {
        hashmap_put(&obj->methods, name, fn);
    }
}

void nat_define_singleton_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *block)) {
    hashmap_put(&obj->singleton_methods, name, fn);
}

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args, NatBlock *block) { // FIXME: kwargs
    assert(receiver);
    if (receiver->type == NAT_VALUE_CLASS) {
        // TODO: instances can have singleton methods too
        NatObject* (*singleton_method)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock *) = hashmap_get(&receiver->singleton_methods, sym);
        if (singleton_method) {
            return singleton_method(env, receiver, argc, args, NULL, block);
        }
        NatObject *class = receiver;
        while (1) {
            class = class->superclass;
            if (class == NULL || class->type != NAT_VALUE_CLASS) break;
            singleton_method = hashmap_get(&class->singleton_methods, sym);
            if (singleton_method) {
                return singleton_method(env, receiver, argc, args, NULL, block);
            }
            if (nat_is_top_class(class)) break;
        }
        fprintf(stderr, "Error: undefined method \"%s\" for %s\n", sym, receiver->class_name);
        abort();
    } else {
        NatObject *class = receiver->class;
        return nat_call_method_on_class(env, class, class, sym, receiver, argc, args, NULL, block); // FIXME: kwargs
    }
}

NatObject *nat_call_method_on_class(NatEnv *env, NatObject *class, NatObject *instance_class, char *method_name, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(class != NULL);
    assert(class->type == NAT_VALUE_CLASS);

    NatObject* (*method)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock*) = hashmap_get(&class->methods, method_name);
    if (method) {
        return method(env, self, argc, args, NULL, block);
    }

    for (size_t i=0; i<class->included_modules_count; i++) {
        NatObject *module = class->included_modules[i];
        method = hashmap_get(&module->methods, method_name);
        if (method) {
            return method(env, self, argc, args, NULL, block);
        }
    }

    if (nat_is_top_class(class)) {
        fprintf(stderr, "Error: undefined method \"%s\" for %s\n", method_name, instance_class->class_name);
        abort();
    }

    return nat_call_method_on_class(env, class->superclass, instance_class, method_name, self, argc, args, kwargs, block);
}

NatObject *nat_lookup_or_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args, NatBlock *block) {
    if (argc > 0) {
        return nat_send(env, receiver, sym, argc, args, block);
    } else {
        NatObject *obj = env_get(env, sym);
        if (obj) {
            return obj;
        } else {
            return nat_send(env, receiver, sym, argc, args, block);
        }
    }
}

NatBlock *nat_block(NatEnv *env, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*, NatBlock*)) {
    NatBlock *block = malloc(sizeof(NatBlock));
    block->env = env;
    block->fn = fn;
    return block;
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
