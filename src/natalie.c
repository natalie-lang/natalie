#include "natalie.h"

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
    env = env_find(env, key);
    if (!env) {
        return NULL;
    }
    NatObject *val = hashmap_get(&env->data, key);
    if (val) {
        return val;
    } else {
        return NULL;
    }
}

NatObject *env_set(NatEnv *env, char *key, NatObject *val) {
    //if (is_blank_line(val)) return val;
    hashmap_remove(&env->data, key);
    hashmap_put(&env->data, key, val);
    return val;
}

NatEnv *build_env(NatEnv *outer) {
    NatEnv *env = malloc(sizeof(NatEnv));
    env->outer = outer;
    hashmap_init(&env->data, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&env->data, hashmap_alloc_key_string, NULL);
    return env;
}

#define TRUE 1
#define FALSE 0

char *heap_string(char *str) {
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

NatObject *nat_alloc() {
    NatObject *val = malloc(sizeof(NatObject));
    val->flags = 0;
    val->type = NAT_VALUE_OTHER;
    val->included_modules_count = 0;
    val->included_modules = NULL;
    hashmap_init(&val->singleton_methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->singleton_methods, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_subclass(NatObject *superclass, char *name) {
    NatObject *val = nat_alloc();
    val->type = NAT_VALUE_CLASS;
    val->class = superclass->class;
    val->class_name = name ? heap_string(name) : NULL;
    val->superclass = superclass;
    hashmap_init(&val->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->methods, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_module(NatEnv *env, char *name) {
    NatObject *val = nat_alloc();
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

NatObject *nat_new(NatObject *class) {
    NatObject *val = nat_alloc();
    val->class = class;
    return val;
}

NatObject *nat_integer(NatEnv *env, int64_t integer) {
    NatObject *obj = nat_new(env_get(env, "Integer"));
    obj->type = NAT_VALUE_INTEGER;
    obj->integer = integer;
    return obj;
}

NatObject *nat_string(NatEnv *env, char *str) {
    NatObject *obj = nat_new(env_get(env, "String"));
    obj->type = NAT_VALUE_STRING;
    size_t len = strlen(str);
    obj->str = heap_string(str);
    obj->str_len = len;
    obj->str_cap = len;
    return obj;
}

NatObject *nat_array(NatEnv *env) {
    NatObject *obj = nat_new(env_get(env, "Array"));
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
    snprintf(str, INT_64_MAX_CHAR_LEN, "%lli", num);
    return str;
  }
}

void nat_define_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*)) {
    if (nat_is_main_object(obj)) {
        hashmap_put(&obj->class->methods, name, fn);
    } else {
        hashmap_put(&obj->methods, name, fn);
    }
}

void nat_define_singleton_method(NatObject *obj, char *name, NatObject* (*fn)(NatEnv*, NatObject*, size_t, NatObject**, struct hashmap*)) {
    hashmap_put(&obj->singleton_methods, name, fn);
}

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args) {
    assert(receiver);
    if (receiver->type == NAT_VALUE_CLASS) {
        // TODO: instances can have singleton methods too
        NatObject* (*singleton_method)(NatEnv*, NatObject*, size_t, NatObject**) = hashmap_get(&receiver->singleton_methods, sym);
        if (singleton_method) {
            return singleton_method(env, receiver, argc, args);
        }
        NatObject *class = receiver;
        while (1) {
            class = class->superclass;
            if (class == NULL || class->type != NAT_VALUE_CLASS) break;
            singleton_method = hashmap_get(&class->singleton_methods, sym);
            if (singleton_method) {
                return singleton_method(env, receiver, argc, args);
            }
            if (nat_is_top_class(class)) break;
        }
        fprintf(stderr, "Error: undefined method \"%s\" for %s\n", sym, receiver->class_name);
        abort();
    } else {
        NatObject *class = receiver->class;
        return nat_call_method_on_class(env, class, class, sym, receiver, argc, args);
    }
}

NatObject *nat_call_method_on_class(NatEnv *env, NatObject *class, NatObject *instance_class, char *method_name, NatObject *self, size_t argc, NatObject **args) {
    assert(class != NULL);
    assert(class->type == NAT_VALUE_CLASS);

    NatObject* (*method)(NatEnv*, NatObject*, size_t, NatObject**) = hashmap_get(&class->methods, method_name);
    if (method) {
        return method(env, self, argc, args);
    }

    for (size_t i=0; i<class->included_modules_count; i++) {
        NatObject *module = class->included_modules[i];
        method = hashmap_get(&module->methods, method_name);
        if (method) {
            return method(env, self, argc, args);
        }
    }

    if (nat_is_top_class(class)) {
        fprintf(stderr, "Error: undefined method \"%s\" for %s\n", method_name, instance_class->class_name);
        abort();
    }

    return nat_call_method_on_class(env, class->superclass, instance_class, method_name, self, argc, args);
}

NatObject *nat_lookup_or_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args) {
    if (argc > 0) {
        return nat_send(env, receiver, sym, argc, args);
    } else {
        NatObject *obj = env_get(env, sym);
        if (obj) {
            return obj;
        } else {
            return nat_send(env, receiver, sym, argc, args);
        }
    }
}

// "0x" + up to 16 hex chars + NULL terminator
#define OBJECT_ID_LEN 2 + 16 + 1

char *nat_object_id(NatObject *obj) {
    char *id = malloc(OBJECT_ID_LEN);
    snprintf(id, OBJECT_ID_LEN, "%p", obj);
    return id;
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
