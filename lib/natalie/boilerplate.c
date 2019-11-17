#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

typedef struct NatObject NatObject;

enum NatValueType {
    NAT_VALUE_STRING,
    NAT_VALUE_NUMBER,
    NAT_VALUE_OTHER
};

struct NatObject {
    enum NatValueType type;
    NatObject *class;
    int is_class;
    char *name;
    NatObject *superclass;

    union {
        long long number;
        //struct hashmap hashmap;

        // NAT_ARRAY_TYPE
        struct {
            size_t ary_len;
            size_t ary_cap;
            NatObject **ary;
        };

        // NAT_STRING_TYPE
        struct {
            size_t str_len;
            size_t str_cap;
            char *str;
        };

        // NAT_REGEX_TYPE
        struct {
            size_t regex_len;
            char *regex;
        };

        // NAT_LAMBDA_TYPE, NAT_CONTINUATION_TYPE
        //struct {
        //  NatObject* (*fn)(NatEnv *env, size_t argc, NatObject **args);
        //  char *function_name;
        //  NatEnv *env;
        //  size_t argc;
        //  NatObject **args;
        //};
    };
};

typedef struct NatEnv NatEnv;

struct NatEnv {
  struct hashmap data;
  NatEnv *outer;
};

NatEnv* build_env(NatEnv *outer) {
  NatEnv *env = malloc(sizeof(NatEnv));
  env->outer = outer;
  hashmap_init(&env->data, hashmap_hash_string, hashmap_compare_string, 100);
  hashmap_set_key_alloc_funcs(&env->data, hashmap_alloc_key_string, NULL);
  return env;
}

NatEnv* build_top_env() {
  NatEnv *top_env = build_env(NULL);
  return top_env;
}

NatEnv* env_find(NatEnv *env, char *key) {
  if (hashmap_get(&env->data, key)) {
    return env;
  } else if (env->outer) {
    return env_find(env->outer, key);
  } else {
    return NULL;
  }
}

NatObject* env_get(NatEnv *env, char *key) {
  env = env_find(env, key);
  /*
  if (!env) {
    return mal_error(mal_sprintf("'%s' not found", key));
  }
  */
  NatObject *val = hashmap_get(&env->data, key);
  if (val) {
    return val;
  } else {
    //return mal_error(mal_sprintf("'%s' not found", key));
  }
}

NatObject* env_set(NatEnv *env, char *key, NatObject *val) {
  //if (is_blank_line(val)) return val;
  hashmap_remove(&env->data, key);
  hashmap_put(&env->data, key, val);
  return val;
}

void env_delete(NatEnv *env, char *key) {
  hashmap_remove(&env->data, key);
}

#define TRUE 1
#define FALSE 0

char *heap_string(char *str) {
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

NatObject* nat_alloc() {
    NatObject *val = malloc(sizeof(NatObject));
    val->type = NAT_VALUE_OTHER;
    return val;
}

NatObject* nat_subclass(NatObject *superclass, char *name) {
    NatObject *val = nat_alloc();
    val->is_class = TRUE;
    val->name = heap_string(name);
    val->superclass = superclass;
    return val;
}

NatObject* nat_new(NatObject *class) {
    NatObject *val = nat_alloc();
    val->is_class = FALSE;
    val->class = class;
    return val;
}

NatObject* nat_number(NatEnv *env, long long num) {
    NatObject *val = nat_new(env_get(env, "Numeric"));
    val->type = NAT_VALUE_NUMBER;
    val->number = num;
    return val;
}

NatObject* nat_string(NatEnv *env, char *str) {
    NatObject *val = nat_new(env_get(env, "String"));
    val->type = NAT_VALUE_STRING;
    size_t len = strlen(str);
    val->str = heap_string(str);
    val->str_len = len;
    val->str_cap = len;
    return val;
}

int main() {
    NatEnv *env = build_top_env();
    NatObject *Class = nat_alloc();
    Class->is_class = TRUE;
    Class->name = heap_string("Class");
    Class->superclass = Class;
    env_set(env, "Class", Class);
    NatObject *Object = nat_subclass(Class, "Object");
    env_set(env, "Object", Object);
    NatObject *main = nat_new(Object);
    NatObject *Numeric = nat_subclass(Object, "Numeric");
    Numeric->type = NAT_VALUE_NUMBER;
    env_set(env, "Numeric", Numeric);
    NatObject *String = nat_subclass(Object, "String");
    String->type = NAT_VALUE_STRING;
    env_set(env, "String", String);
    /*MAIN*/
    return 0;
}