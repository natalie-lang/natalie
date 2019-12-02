#include "natalie.h"
#include "nat_array.h"
#include "nat_class.h"
#include "nat_integer.h"
#include "nat_kernel.h"
#include "nat_module.h"
#include "nat_nil_class.h"
#include "nat_object.h"
#include "nat_string.h"
#include "nat_symbol.h"

NatEnv *build_top_env() {
    NatEnv *env = build_env(NULL);

    NatObject *Class = nat_alloc();
    Class->flags = NAT_FLAG_TOP_CLASS;
    Class->type = NAT_VALUE_CLASS;
    Class->class_name = heap_string("Class");
    Class->superclass = Class;
    Class->class = Class;
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, NULL);
    nat_define_singleton_method(Class, "new", Class_new);
    nat_define_singleton_method(Class, "inspect", Class_inspect);
    nat_define_singleton_method(Class, "include", Class_include);
    nat_define_singleton_method(Class, "included_modules", Class_included_modules);
    env_set(env, "Class", Class);

    NatObject *Kernel = nat_module(env, "Kernel");
    nat_define_method(Kernel, "puts", Kernel_puts);
    nat_define_method(Kernel, "print", Kernel_print);
    nat_define_method(Kernel, "p", Kernel_p);

    NatObject *Object = nat_subclass(Class, "Object");
    nat_class_include(Object, Kernel);
    nat_define_method(Object, "inspect", Object_inspect);
    nat_define_method(Object, "object_id", Object_object_id);
    nat_define_singleton_method(Object, "new", Object_new);
    env_set(env, "Object", Object);

    NatObject *Symbol = nat_subclass(Object, "Symbol");
    nat_define_method(Symbol, "to_s", Symbol_to_s);
    nat_define_method(Symbol, "inspect", Symbol_inspect);
    env_set(env, "Symbol", Symbol);

    NatObject *Module = nat_subclass(Class, "Module");
    nat_define_method(Module, "inspect", Module_inspect);
    nat_define_singleton_method(Module, "new", Module_new);
    env_set(env, "Module", Module);

    NatObject *main_obj = nat_new(Object);
    main_obj->flags = NAT_FLAG_MAIN_OBJECT;
    env_set(env, "self", main_obj);

    NatObject *NilClass = nat_subclass(Object, "NilClass");
    nat_define_method(NilClass, "to_s", NilClass_to_s);
    nat_define_method(NilClass, "inspect", NilClass_inspect);
    env_set(env, "NilClass", NilClass);

    NatObject *nil = nat_new(NilClass);
    nil->type = NAT_VALUE_NIL;
    env_set(env, "nil", nil);

    NatObject *Numeric = nat_subclass(Object, "Numeric");
    env_set(env, "Numeric", Numeric);

    NatObject *Integer = nat_subclass(Numeric, "Integer");
    nat_define_method(Integer, "to_s", Integer_to_s);
    nat_define_method(Integer, "inspect", Integer_to_s);
    nat_define_method(Integer, "+", Integer_add);
    nat_define_method(Integer, "-", Integer_sub);
    nat_define_method(Integer, "*", Integer_mul);
    nat_define_method(Integer, "/", Integer_div);
    env_set(env, "Integer", Integer);

    NatObject *String = nat_subclass(Object, "String");
    nat_define_method(String, "to_s", String_to_s);
    nat_define_method(String, "inspect", String_inspect);
    nat_define_method(String, "<<", String_ltlt);
    nat_define_method(String, "+", String_add);
    env_set(env, "String", String);

    NatObject *Array = nat_subclass(Object, "Array");
    nat_define_method(Array, "inspect", Array_inspect);
    nat_define_method(Array, "<<", Array_ltlt);
    nat_define_method(Array, "+", Array_add);
    nat_define_method(Array, "[]", Array_ref);
    nat_define_method(Array, "size", Array_size);
    nat_define_method(Array, "length", Array_size);
    env_set(env, "Array", Array);

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
