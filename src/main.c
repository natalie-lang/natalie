#include <setjmp.h>

#include "natalie.h"
#include "nat_array.h"
#include "nat_basic_object.h"
#include "nat_class.h"
#include "nat_comparable.h"
#include "nat_exception.h"
#include "nat_false_class.h"
#include "nat_hash.h"
#include "nat_integer.h"
#include "nat_kernel.h"
#include "nat_main_obj.h"
#include "nat_module.h"
#include "nat_nil_class.h"
#include "nat_object.h"
#include "nat_proc.h"
#include "nat_string.h"
#include "nat_symbol.h"
#include "nat_true_class.h"

NatObject *obj_language_exceptions(NatEnv *env, NatObject *self);

NatEnv *build_top_env() {
    NatEnv *env = nat_build_env(NULL);
    env->method_name = heap_string("<main>");

    NatObject *Class = nat_alloc(env);
    Class->type = NAT_VALUE_CLASS;
    Class->class_name = heap_string("Class");
    Class->class = Class;
    Class->env = nat_build_env(env);
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, NULL);
    nat_define_method(Class, "superclass", Class_superclass);
    nat_var_set(env, "Class", Class);

    NatObject *BasicObject = nat_alloc(env);
    BasicObject->type = NAT_VALUE_CLASS;
    BasicObject->class_name = heap_string("BasicObject");
    BasicObject->class = Class;
    BasicObject->env = nat_build_env(env);
    BasicObject->superclass = NULL;
    hashmap_init(&BasicObject->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&BasicObject->methods, hashmap_alloc_key_string, NULL);
    nat_define_method(BasicObject, "!", BasicObject_not);
    nat_define_method(BasicObject, "!", BasicObject_not);
    nat_define_method(BasicObject, "==", BasicObject_eqeq);
    nat_define_method(BasicObject, "!=", BasicObject_neq);
    nat_define_method(BasicObject, "equal?", Kernel_equal);
    nat_define_method(BasicObject, "instance_eval", BasicObject_instance_eval);
    nat_var_set(env, "BasicObject", BasicObject);

    NatObject *Object = nat_subclass(env, BasicObject, "Object");
    nat_var_set(env, "Object", Object);
    nat_define_singleton_method(env, Object, "new", Object_new);

    // these must be defined after Object exists
    nat_define_singleton_method(env, Class, "new", Class_new);
    BasicObject->singleton_class = Class->singleton_class;

    NatObject *Module = nat_subclass(env, Object, "Module");
    Class->superclass = Module;
    nat_define_method(Module, "inspect", Module_inspect);
    nat_define_singleton_method(env, Module, "new", Module_new);
    nat_define_method(Module, "name", Module_name);
    nat_define_method(Module, "===", Module_eqeqeq);
    nat_define_method(Module, "ancestors", Module_ancestors);
    nat_define_method(Module, "attr", Module_attr_reader);
    nat_define_method(Module, "attr_reader", Module_attr_reader);
    nat_define_method(Module, "attr_writer", Module_attr_writer);
    nat_define_method(Module, "attr_accessor", Module_attr_accessor);
    nat_define_method(Module, "include", Module_include);
    nat_define_method(Module, "included_modules", Module_included_modules);
    nat_define_method(Module, "define_method", Module_define_method);
    nat_define_method(Module, "class_eval", Module_class_eval);
    nat_var_set(env, "Module", Module);

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
    nat_define_method(Kernel, "singleton_class", Kernel_singleton_class);
    nat_define_method(Kernel, "instance_variable_get", Kernel_instance_variable_get);
    nat_define_method(Kernel, "instance_variable_set", Kernel_instance_variable_set);
    nat_define_method(Kernel, "raise", Kernel_raise);
    nat_define_method(Kernel, "exit", Kernel_exit);
    nat_define_method(Kernel, "at_exit", Kernel_at_exit);
    nat_define_method(Kernel, "respond_to?", Kernel_respond_to);
    nat_define_method(Kernel, "dup", Kernel_dup);
    nat_define_method(Kernel, "methods", Kernel_methods);
    nat_define_method(Kernel, "public_methods", Kernel_methods); // TODO
    nat_define_method(Kernel, "is_a?", Kernel_is_a);
    nat_define_method(Kernel, "hash", Kernel_hash);
    nat_define_method(Kernel, "proc", Kernel_proc);

    NatObject *Comparable = nat_module(env, "Comparable");
    COMPARABLE_INIT();
    nat_var_set(env, "Comparable", Comparable);

    NatObject *Symbol = nat_subclass(env, Object, "Symbol");
    nat_define_method(Symbol, "to_s", Symbol_to_s);
    nat_define_method(Symbol, "inspect", Symbol_inspect);
    nat_var_set(env, "Symbol", Symbol);

    NatObject *main_obj = nat_new(env, Object, 0, NULL, NULL, NULL);
    main_obj->flags = NAT_FLAG_MAIN_OBJECT;
    nat_define_singleton_method(env, main_obj, "inspect", main_obj_inspect);
    NatObject *self = main_obj;
    nat_var_set(env, "self", self);

    NatObject *NilClass = nat_subclass(env, Object, "NilClass");
    nat_define_singleton_method(env, NilClass, "new", NilClass_new);
    nat_define_method(NilClass, "to_s", NilClass_to_s);
    nat_define_method(NilClass, "to_a", NilClass_to_a);
    nat_define_method(NilClass, "inspect", NilClass_inspect);
    nat_var_set(env, "NilClass", NilClass);

    NatObject *nil = nat_new(env, NilClass, 0, NULL, NULL, NULL);
    nil->type = NAT_VALUE_NIL;
    nat_var_set(env, "nil", nil);

    NatObject *TrueClass = nat_subclass(env, Object, "TrueClass");
    nat_define_singleton_method(env, TrueClass, "new", TrueClass_new);
    nat_define_method(TrueClass, "to_s", TrueClass_to_s);
    nat_define_method(TrueClass, "inspect", TrueClass_to_s);
    nat_var_set(env, "TrueClass", TrueClass);

    NatObject *true_obj = nat_new(env, TrueClass, 0, NULL, NULL, NULL);
    true_obj->type = NAT_VALUE_TRUE;
    nat_var_set(env, "true", true_obj);

    NatObject *FalseClass = nat_subclass(env, Object, "FalseClass");
    nat_define_singleton_method(env, FalseClass, "new", FalseClass_new);
    nat_define_method(FalseClass, "to_s", FalseClass_to_s);
    nat_define_method(FalseClass, "inspect", FalseClass_to_s);
    nat_var_set(env, "FalseClass", FalseClass);

    NatObject *false_obj = nat_new(env, FalseClass, 0, NULL, NULL, NULL);
    false_obj->type = NAT_VALUE_FALSE;
    nat_var_set(env, "false", false_obj);

    NatObject *Numeric = nat_subclass(env, Object, "Numeric");
    nat_class_include(Numeric, Comparable);
    nat_var_set(env, "Numeric", Numeric);

    NatObject *Integer = nat_subclass(env, Numeric, "Integer");
    nat_define_method(Integer, "to_s", Integer_to_s);
    nat_define_method(Integer, "inspect", Integer_to_s);
    nat_define_method(Integer, "+", Integer_add);
    nat_define_method(Integer, "-", Integer_sub);
    nat_define_method(Integer, "*", Integer_mul);
    nat_define_method(Integer, "/", Integer_div);
    nat_define_method(Integer, "<=>", Integer_cmp);
    nat_define_method(Integer, "===", Integer_eqeqeq);
    nat_define_method(Integer, "times", Integer_times);
    nat_var_set(env, "Integer", Integer);

    NatObject *String = nat_subclass(env, Object, "String");
    nat_class_include(String, Comparable);
    nat_define_method(String, "to_s", String_to_s);
    nat_define_method(String, "inspect", String_inspect);
    nat_define_method(String, "<=>", String_cmp);
    nat_define_method(String, "<<", String_ltlt);
    nat_define_method(String, "+", String_add);
    nat_define_method(String, "*", String_mul);
    nat_define_method(String, "==", String_eqeq);
    nat_define_method(String, "===", String_eqeq);
    nat_var_set(env, "String", String);

    NatObject *Array = nat_subclass(env, Object, "Array");
    nat_define_method(Array, "inspect", Array_inspect);
    nat_define_method(Array, "<<", Array_ltlt);
    nat_define_method(Array, "+", Array_add);
    nat_define_method(Array, "-", Array_sub);
    nat_define_method(Array, "[]", Array_ref);
    nat_define_method(Array, "[]=", Array_refeq);
    nat_define_method(Array, "size", Array_size);
    nat_define_method(Array, "any?", Array_any);
    nat_define_method(Array, "length", Array_size);
    nat_define_method(Array, "==", Array_eqeq);
    nat_define_method(Array, "===", Array_eqeq);
    nat_define_method(Array, "each", Array_each);
    nat_define_method(Array, "map", Array_map);
    nat_define_method(Array, "first", Array_first);
    nat_define_method(Array, "last", Array_last);
    nat_define_method(Array, "to_ary", Array_to_ary);
    nat_define_method(Array, "pop", Array_pop);
    nat_define_method(Array, "include?", Array_include);
    nat_var_set(env, "Array", Array);

    NatObject *Hash = nat_subclass(env, Object, "Hash");
    nat_define_method(Hash, "inspect", Hash_inspect);
    nat_define_method(Hash, "[]", Hash_ref);
    nat_define_method(Hash, "[]=", Hash_refeq);
    nat_define_method(Hash, "delete", Hash_delete);
    nat_define_method(Hash, "size", Hash_size);
    nat_define_method(Hash, "==", Hash_eqeq);
    nat_define_method(Hash, "===", Hash_eqeq);
    nat_define_method(Hash, "each", Hash_each);
    nat_define_method(Hash, "keys", Hash_keys);
    nat_define_method(Hash, "values", Hash_values);
    nat_var_set(env, "Hash", Hash);

    NatObject *Proc = nat_subclass(env, Object, "Proc");
    nat_define_method(Proc, "call", Proc_call);
    nat_define_method(Proc, "lambda?", Proc_lambda);
    nat_var_set(env, "Proc", Proc);

    NatObject *Exception = nat_subclass(env, Object, "Exception");
    nat_var_set(env, "Exception", Exception);
    nat_define_method(Exception, "initialize", Exception_initialize);
    nat_define_method(Exception, "inspect", Exception_inspect);
    nat_define_method(Exception, "message", Exception_message);
    nat_define_method(Exception, "backtrace", Exception_backtrace);
    nat_define_singleton_method(env, Exception, "new", Exception_new);
    NatObject *StandardError = nat_subclass(env, Exception, "StandardError");
    nat_var_set(env, "StandardError", StandardError);
    NatObject *NameError = nat_subclass(env, StandardError, "NameError");
    nat_var_set(env, "NameError", NameError);
    NatObject *NoMethodError = nat_subclass(env, NameError, "NoMethodError");
    nat_var_set(env, "NoMethodError", NoMethodError);
    NatObject *ArgumentError = nat_subclass(env, StandardError, "ArgumentError");
    nat_var_set(env, "ArgumentError", ArgumentError);
    NatObject *RuntimeError = nat_subclass(env, StandardError, "RuntimeError");
    nat_var_set(env, "RuntimeError", RuntimeError);
    NatObject *TypeError = nat_subclass(env, StandardError, "TypeError");
    nat_var_set(env, "TypeError", TypeError);

    nat_global_set(env, "$NAT_at_exit_handlers", nat_array(env));

    obj_language_exceptions(env, self);

    return env;
}

/*TOP*/

NatObject *EVAL(NatEnv *env) {
    NatObject *self = nat_var_get(env, "self");
    UNUSED(self); // maybe unused
    int run_exit_handlers = TRUE;
    if (!NAT_RESCUE(env)) {
        /*BODY*/
        run_exit_handlers = FALSE;
        nat_run_at_exit_handlers(env);
        return nat_var_get(env, "nil"); // just in case there's no return value
    } else {
        nat_handle_top_level_exception(env, run_exit_handlers);
        return nat_var_get(env, "nil");
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 1024);
    NatEnv *env = build_top_env();
    if (EVAL(env)) {
        return 0;
    } else {
        return 1;
    }
}
