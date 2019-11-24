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
    hashmap_init(&val->singleton_methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->singleton_methods, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_subclass(NatObject *superclass, char *name) {
    NatObject *val = nat_alloc();
    val->type = NAT_VALUE_CLASS;
    val->class_name = heap_string(name);
    val->superclass = superclass;
    hashmap_init(&val->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&val->methods, hashmap_alloc_key_string, NULL);
    return val;
}

NatObject *nat_new(NatObject *class) {
    NatObject *val = nat_alloc();
    val->class = class;
    return val;
}

NatObject *nat_number(NatEnv *env, long long num) {
    NatObject *val = nat_new(env_get(env, "Numeric"));
    val->type = NAT_VALUE_NUMBER;
    val->number = num;
    return val;
}

NatObject *nat_string(NatEnv *env, char *str) {
    NatObject *val = nat_new(env_get(env, "String"));
    val->type = NAT_VALUE_STRING;
    size_t len = strlen(str);
    val->str = heap_string(str);
    val->str_len = len;
    val->str_cap = len;
    return val;
}

// note: there is a formula using log10 to calculate a number length, but this works for now
size_t num_char_len(long long num) {
    if (num < 0) {
        return 1 + num_char_len(llabs(num));
    } else if (num < 10) {
        return 1;
    } else if (num < 100) {
        return 2;
    } else if (num < 1000) {
        return 3;
    } else if (num < 10000) {
        return 4;
    } else if (num < 100000) {
        return 5;
    } else if (num < 1000000) {
        return 6;
    } else if (num < 1000000000) {
        return 9;
    } else if (num < 1000000000000) {
        return 12;
    } else if (num < 1000000000000000) {
        return 15;
    } else if (num < 1000000000000000000) {
        return 18;
    } else {
        // up to 128 bits
        return 40;
    }
}

char* long_long_to_string(long long num) {
  char* str;
  size_t len;
  if (num == 0) {
    return heap_string("0");
  } else {
    len = num_char_len(num);
    str = malloc(len + 1);
    snprintf(str, len + 1, "%lli", num);
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

NatObject *nat_send(NatEnv *env, NatObject *receiver, char *sym, size_t argc, NatObject **args) {
    assert(receiver);
    if (receiver->type == NAT_VALUE_CLASS) {
        // TODO: instances can have singleton methods too
        NatObject* (*singleton_method)(NatEnv*, NatObject*, size_t, NatObject**) = hashmap_get(&receiver->singleton_methods, sym);
        if (singleton_method) {
            return singleton_method(env, receiver, argc, args);
        }
        NatObject *klass = receiver;
        while (1) {
            klass = klass->superclass;
            if (klass == NULL || klass->type != NAT_VALUE_CLASS) break;
            singleton_method = hashmap_get(&klass->singleton_methods, sym);
            if (singleton_method) {
                return singleton_method(env, receiver, argc, args);
            }
            if (nat_is_top_class(klass)) break;
        }
        fprintf(stderr, "Error: undefined method \"%s\" for %s\n", sym, receiver->class_name);
        abort();
    } else {
        NatObject* (*method)(NatEnv*, NatObject*, size_t, NatObject**) = hashmap_get(&receiver->class->methods, sym);
        if (method) {
            return method(env, receiver, argc, args);
        }
        NatObject *klass = receiver->class;
        while (1) {
            klass = klass->superclass;
            if (klass == NULL || klass->type != NAT_VALUE_CLASS) break;
            method = hashmap_get(&klass->singleton_methods, sym);
            if (method) {
                return method(env, receiver, argc, args);
            }
            if (nat_is_top_class(klass)) break;
        }
        fprintf(stderr, "Error: undefined method \"%s\" for %s\n", sym, receiver->class->class_name);
        abort();
    }
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
    if (capacity > 0 && min_capacity <= capacity * STRING_GROW_FACTOR) {
        nat_grow_string(obj, capacity * STRING_GROW_FACTOR);
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
