#include <setjmp.h>

#include "natalie.h"
#include "nat_array.h"
#include "nat_basic_object.h"
#include "nat_class.h"
#include "nat_comparable.h"
#include "nat_exception.h"
#include "nat_false_class.h"
#include "nat_integer.h"
#include "nat_kernel.h"
#include "nat_module.h"
#include "nat_nil_class.h"
#include "nat_object.h"
#include "nat_string.h"
#include "nat_symbol.h"
#include "nat_true_class.h"

NatEnv *build_top_env() {
    NatEnv *env = build_env(NULL);

    NatObject *Class = nat_alloc(env);
    Class->flags = NAT_FLAG_TOP_CLASS;
    Class->type = NAT_VALUE_CLASS;
    Class->class_name = heap_string("Class");
    Class->class = Class;
    Class->env = build_env(env);
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, NULL);
    nat_define_singleton_method(Class, "new", Class_new);
    nat_define_singleton_method(Class, "inspect", Class_inspect);
    nat_define_singleton_method(Class, "include", Class_include);
    nat_define_singleton_method(Class, "included_modules", Class_included_modules);
    nat_define_singleton_method(Class, "==", Kernel_equal);
    nat_define_singleton_method(Class, "eql?", Kernel_equal);
    nat_define_singleton_method(Class, "equal?", Kernel_equal);
    nat_define_singleton_method(Class, "===", Class_eqeqeq);
    nat_define_singleton_method(Class, "ancestors", Class_ancestors);
    nat_define_singleton_method(Class, "class", Kernel_class);
    nat_define_singleton_method(Class, "superclass", Class_superclass);
    nat_define_singleton_method(Class, "attr_reader", Class_attr_reader);
    nat_define_singleton_method(Class, "attr_writer", Class_attr_writer);
    nat_define_singleton_method(Class, "attr_accessor", Class_attr_accessor);
    env_set(env, "Class", Class);

    NatObject *BasicObject = nat_alloc(env);
    BasicObject->flags = NAT_FLAG_TOP_CLASS;
    BasicObject->type = NAT_VALUE_CLASS;
    BasicObject->class_name = heap_string("BasicObject");
    BasicObject->class = Class;
    BasicObject->singleton_methods = Class->singleton_methods;
    BasicObject->env = build_env(env);
    hashmap_init(&BasicObject->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&BasicObject->methods, hashmap_alloc_key_string, NULL);
    nat_define_method(BasicObject, "!", BasicObject_not);
    nat_define_method(BasicObject, "==", Kernel_equal);
    nat_define_method(BasicObject, "equal?", Kernel_equal);
    env_set(env, "BasicObject", BasicObject);

    NatObject *Object = nat_subclass(env, BasicObject, "Object");
    nat_define_singleton_method(Object, "new", Object_new);
    env_set(env, "Object", Object);

    NatObject *Module = nat_subclass(env, Object, "Module");
    Class->superclass = Module;
    nat_define_method(Module, "inspect", Module_inspect);
    nat_define_singleton_method(Module, "new", Module_new);
    env_set(env, "Module", Module);

    NatObject *Kernel = nat_module(env, "Kernel");
    nat_class_include(Object, Kernel);
    nat_define_method(Kernel, "puts", Kernel_puts);
    nat_define_method(Kernel, "print", Kernel_print);
    nat_define_method(Kernel, "p", Kernel_p);
    nat_define_method(Kernel, "inspect", Kernel_inspect);
    nat_define_method(Kernel, "object_id", Kernel_object_id);
    nat_define_method(Kernel, "===", Kernel_equal);
    nat_define_method(Kernel, "eql?", Kernel_equal);
    nat_define_method(Kernel, "class", Kernel_class);
    nat_define_method(Kernel, "instance_variable_get", Kernel_instance_variable_get);
    nat_define_method(Kernel, "instance_variable_set", Kernel_instance_variable_set);
    nat_define_method(Kernel, "raise", Kernel_raise);

    NatObject *Comparable = nat_module(env, "Comparable");
    COMPARABLE_INIT();
    env_set(env, "Comparable", Comparable);

    NatObject *Symbol = nat_subclass(env, Object, "Symbol");
    nat_define_method(Symbol, "to_s", Symbol_to_s);
    nat_define_method(Symbol, "inspect", Symbol_inspect);
    env_set(env, "Symbol", Symbol);

    NatObject *main_obj = nat_new(env, Object, 0, NULL, NULL, NULL);
    main_obj->flags = NAT_FLAG_MAIN_OBJECT;
    env_set(env, "self", main_obj);

    NatObject *NilClass = nat_subclass(env, Object, "NilClass");
    nat_define_singleton_method(NilClass, "new", NilClass_new);
    nat_define_method(NilClass, "to_s", NilClass_to_s);
    nat_define_method(NilClass, "inspect", NilClass_inspect);
    env_set(env, "NilClass", NilClass);

    NatObject *nil = nat_new(env, NilClass, 0, NULL, NULL, NULL);
    nil->type = NAT_VALUE_NIL;
    env_set(env, "nil", nil);

    NatObject *TrueClass = nat_subclass(env, Object, "TrueClass");
    nat_define_singleton_method(TrueClass, "new", TrueClass_new);
    nat_define_method(TrueClass, "to_s", TrueClass_to_s);
    nat_define_method(TrueClass, "inspect", TrueClass_to_s);
    env_set(env, "TrueClass", TrueClass);

    NatObject *true_obj = nat_new(env, TrueClass, 0, NULL, NULL, NULL);
    true_obj->type = NAT_VALUE_TRUE;
    env_set(env, "true", true_obj);

    NatObject *FalseClass = nat_subclass(env, Object, "FalseClass");
    nat_define_singleton_method(FalseClass, "new", FalseClass_new);
    nat_define_method(FalseClass, "to_s", FalseClass_to_s);
    nat_define_method(FalseClass, "inspect", FalseClass_to_s);
    env_set(env, "FalseClass", FalseClass);

    NatObject *false_obj = nat_new(env, FalseClass, 0, NULL, NULL, NULL);
    false_obj->type = NAT_VALUE_FALSE;
    env_set(env, "false", false_obj);

    NatObject *Numeric = nat_subclass(env, Object, "Numeric");
    nat_class_include(Numeric, Comparable);
    env_set(env, "Numeric", Numeric);

    NatObject *Integer = nat_subclass(env, Numeric, "Integer");
    nat_define_method(Integer, "to_s", Integer_to_s);
    nat_define_method(Integer, "inspect", Integer_to_s);
    nat_define_method(Integer, "+", Integer_add);
    nat_define_method(Integer, "-", Integer_sub);
    nat_define_method(Integer, "*", Integer_mul);
    nat_define_method(Integer, "/", Integer_div);
    nat_define_method(Integer, "<=>", Integer_cmp);
    nat_define_method(Integer, "===", Integer_eqeqeq);
    env_set(env, "Integer", Integer);

    NatObject *String = nat_subclass(env, Object, "String");
    nat_define_method(String, "to_s", String_to_s);
    nat_define_method(String, "inspect", String_inspect);
    nat_define_method(String, "<<", String_ltlt);
    nat_define_method(String, "+", String_add);
    nat_define_method(String, "==", String_eqeq);
    nat_define_method(String, "===", String_eqeq);
    env_set(env, "String", String);

    NatObject *Array = nat_subclass(env, Object, "Array");
    nat_define_method(Array, "inspect", Array_inspect);
    nat_define_method(Array, "<<", Array_ltlt);
    nat_define_method(Array, "+", Array_add);
    nat_define_method(Array, "[]", Array_ref);
    nat_define_method(Array, "[]=", Array_refeq);
    nat_define_method(Array, "size", Array_size);
    nat_define_method(Array, "length", Array_size);
    nat_define_method(Array, "==", Array_eqeq);
    nat_define_method(Array, "===", Array_eqeq);
    nat_define_method(Array, "each", Array_each);
    nat_define_method(Array, "first", Array_first);
    nat_define_method(Array, "last", Array_last);
    env_set(env, "Array", Array);

    NatObject *Exception = nat_subclass(env, Object, "Exception");
    env_set(env, "Exception", Exception);
    nat_define_method(Exception, "initialize", Exception_initialize);
    nat_define_method(Exception, "inspect", Exception_inspect);
    nat_define_singleton_method(Exception, "new", Exception_new);
    NatObject *StandardError = nat_subclass(env, Exception, "StandardError");
    env_set(env, "StandardError", StandardError);
    NatObject *NameError = nat_subclass(env, StandardError, "NameError");
    env_set(env, "NameError", NameError);
    NatObject *ArgumentError = nat_subclass(env, StandardError, "ArgumentError");
    env_set(env, "ArgumentError", ArgumentError);
    NatObject *RuntimeError = nat_subclass(env, StandardError, "RuntimeError");
    env_set(env, "RuntimeError", RuntimeError);
    NatObject *TypeError = nat_subclass(env, StandardError, "TypeError");
    env_set(env, "TypeError", TypeError);

    return env;
}

/*TOP*/

NatObject *EVAL(NatEnv *env) {
    NatObject *self = env_get(env, "self");
    UNUSED(self); // maybe unused
    if (!NAT_RESCUE(env)) {
        /*DECL*/
        /*BODY*/
    } else {
        NatObject *exception = env->exception;
        assert(exception);
        assert(exception->type == NAT_VALUE_EXCEPTION);
        fflush(stdout);
        fprintf(stderr, "%s\n", exception->message);
        return env_get(env, "nil");
    }
}

int main(void) {
    NatEnv *env = build_top_env();
    EVAL(env);
    return 0;
}
