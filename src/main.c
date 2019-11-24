#include "natalie.h"
#include "nat_class.h"
#include "nat_nil_class.h"
#include "nat_numeric.h"
#include "nat_object.h"
#include "nat_string.h"

NatEnv *build_top_env() {
    NatEnv *env = build_env(NULL);

    NatObject *Class = nat_alloc();
    Class->flags = NAT_FLAG_TOP_CLASS;
    Class->type = NAT_VALUE_CLASS;
    Class->class_name = heap_string("Class");
    Class->superclass = Class;
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, NULL);
    hashmap_put(&Class->singleton_methods, "new", Class_new);
    env_set(env, "Class", Class);

    NatObject *Object = nat_subclass(Class, "Object");
    hashmap_put(&Object->methods, "puts", Object_puts);
    hashmap_put(&Object->methods, "inspect", Object_inspect);
    hashmap_put(&Object->singleton_methods, "new", Object_new);
    env_set(env, "Object", Object);

    NatObject *main_obj = nat_new(Object);
    main_obj->flags = NAT_FLAG_MAIN_OBJECT;
    env_set(env, "self", main_obj);

    NatObject *NilClass = nat_subclass(Object, "NilClass");
    hashmap_put(&NilClass->methods, "to_s", NilClass_to_s);
    hashmap_put(&NilClass->methods, "inspect", NilClass_inspect);
    env_set(env, "NilClass", NilClass);

    NatObject *nil = nat_new(NilClass);
    nil->type = NAT_VALUE_NIL;
    env_set(env, "nil", nil);

    NatObject *Numeric = nat_subclass(Object, "Numeric");
    hashmap_put(&Numeric->methods, "to_s", Numeric_to_s);
    hashmap_put(&Numeric->methods, "inspect", Numeric_to_s);
    hashmap_put(&Numeric->methods, "*", Numeric_mul);
    env_set(env, "Numeric", Numeric);

    NatObject *String = nat_subclass(Object, "String");
    hashmap_put(&String->methods, "to_s", String_to_s);
    hashmap_put(&String->methods, "inspect", String_inspect);
    hashmap_put(&String->methods, "<<", String_ltlt);
    hashmap_put(&String->methods, "+", String_add);
    env_set(env, "String", String);

    return env;
}

/*TOP*/

NatObject *EVAL(NatEnv *env) {
    NatObject *self = env_get(env, "self");
    UNUSED(self); // maybe unused
    /*DECL*/
    /*BODY*/
}

int main(void) {
    NatEnv *env = build_top_env();
    EVAL(env);
    return 0;
}
