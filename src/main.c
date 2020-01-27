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

// built-in constants
NatObject *Object,
          *Integer,
          *nil,
          *true_obj,
          *false_obj;

/* end of front matter */

NatObject *obj_language_exceptions(NatEnv *env, NatObject *self);

NatEnv *build_top_env() {
    NatEnv *env = nat_build_env(NULL);
    env->method_name = heap_string("<main>");

    NatObject *Class = nat_alloc(env);
    Class->type = NAT_VALUE_CLASS;
    Class->class_name = heap_string("Class");
    Class->klass = Class;
    Class->env = nat_build_env(env);
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, NULL);
    hashmap_init(&Class->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&Class->constants, hashmap_alloc_key_string, NULL);
    nat_define_method(Class, "superclass", Class_superclass);

    NatObject *BasicObject = nat_alloc(env);
    BasicObject->type = NAT_VALUE_CLASS;
    BasicObject->class_name = heap_string("BasicObject");
    BasicObject->klass = Class;
    BasicObject->env = nat_build_env(env);
    BasicObject->superclass = NULL;
    hashmap_init(&BasicObject->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&BasicObject->methods, hashmap_alloc_key_string, NULL);
    hashmap_init(&BasicObject->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&BasicObject->constants, hashmap_alloc_key_string, NULL);
    nat_define_method(BasicObject, "!", BasicObject_not);
    nat_define_method(BasicObject, "!", BasicObject_not);
    nat_define_method(BasicObject, "==", BasicObject_eqeq);
    nat_define_method(BasicObject, "!=", BasicObject_neq);
    nat_define_method(BasicObject, "equal?", Kernel_equal);
    nat_define_method(BasicObject, "instance_eval", BasicObject_instance_eval);

    Object = nat_subclass(env, BasicObject, "Object");
    nat_define_singleton_method(env, Object, "new", Object_new);

    // these must be defined after Object exists
    nat_define_singleton_method(env, Class, "new", Class_new);
    BasicObject->singleton_class = Class->singleton_class;
    nat_const_set(env, Object, "Class", Class);
    nat_const_set(env, Object, "BasicObject", BasicObject);
    nat_const_set(env, Object, "Object", Object);

    NatObject *Module = nat_subclass(env, Object, "Module");
    nat_const_set(env, Object, "Module", Module);
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

    NatObject *Kernel = nat_module(env, "Kernel");
    nat_const_set(env, Object, "Kernel", Kernel);
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
    nat_define_method(Kernel, "__method__", Kernel_method);

    NatObject *Comparable = nat_module(env, "Comparable");
    nat_const_set(env, Object, "Comparable", Comparable);
    COMPARABLE_INIT();

    NatObject *Symbol = nat_subclass(env, Object, "Symbol");
    nat_const_set(env, Object, "Symbol", Symbol);
    nat_define_method(Symbol, "to_s", Symbol_to_s);
    nat_define_method(Symbol, "inspect", Symbol_inspect);

    NatObject *NilClass = nat_subclass(env, Object, "NilClass");
    nat_const_set(env, Object, "NilClass", NilClass);
    nat_define_singleton_method(env, NilClass, "new", NilClass_new);
    nat_define_method(NilClass, "to_s", NilClass_to_s);
    nat_define_method(NilClass, "to_a", NilClass_to_a);
    nat_define_method(NilClass, "inspect", NilClass_inspect);

    nil = nat_new(env, NilClass, 0, NULL, NULL, NULL);
    nil->type = NAT_VALUE_NIL;

    NatObject *TrueClass = nat_subclass(env, Object, "TrueClass");
    nat_const_set(env, Object, "TrueClass", TrueClass);
    nat_define_singleton_method(env, TrueClass, "new", TrueClass_new);
    nat_define_method(TrueClass, "to_s", TrueClass_to_s);
    nat_define_method(TrueClass, "inspect", TrueClass_to_s);

    true_obj = nat_new(env, TrueClass, 0, NULL, NULL, NULL);
    true_obj->type = NAT_VALUE_TRUE;

    NatObject *FalseClass = nat_subclass(env, Object, "FalseClass");
    nat_const_set(env, Object, "FalseClass", FalseClass);
    nat_define_singleton_method(env, FalseClass, "new", FalseClass_new);
    nat_define_method(FalseClass, "to_s", FalseClass_to_s);
    nat_define_method(FalseClass, "inspect", FalseClass_to_s);

    false_obj = nat_new(env, FalseClass, 0, NULL, NULL, NULL);
    false_obj->type = NAT_VALUE_FALSE;

    NatObject *Numeric = nat_subclass(env, Object, "Numeric");
    nat_const_set(env, Object, "Numeric", Numeric);
    nat_class_include(Numeric, Comparable);

    Integer = nat_subclass(env, Numeric, "Integer");
    nat_const_set(env, Object, "Integer", Integer);
    nat_define_method(Integer, "to_s", Integer_to_s);
    nat_define_method(Integer, "inspect", Integer_to_s);
    nat_define_method(Integer, "+", Integer_add);
    nat_define_method(Integer, "-", Integer_sub);
    nat_define_method(Integer, "*", Integer_mul);
    nat_define_method(Integer, "/", Integer_div);
    nat_define_method(Integer, "<=>", Integer_cmp);
    nat_define_method(Integer, "===", Integer_eqeqeq);
    nat_define_method(Integer, "times", Integer_times);

    NatObject *String = nat_subclass(env, Object, "String");
    nat_const_set(env, Object, "String", String);
    nat_class_include(String, Comparable);
    nat_define_method(String, "to_s", String_to_s);
    nat_define_method(String, "inspect", String_inspect);
    nat_define_method(String, "<=>", String_cmp);
    nat_define_method(String, "<<", String_ltlt);
    nat_define_method(String, "+", String_add);
    nat_define_method(String, "*", String_mul);
    nat_define_method(String, "==", String_eqeq);
    nat_define_method(String, "===", String_eqeq);

    NatObject *Array = nat_subclass(env, Object, "Array");
    nat_const_set(env, Object, "Array", Array);
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

    NatObject *Hash = nat_subclass(env, Object, "Hash");
    nat_const_set(env, Object, "Hash", Hash);
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

    NatObject *Proc = nat_subclass(env, Object, "Proc");
    nat_const_set(env, Object, "Proc", Proc);
    nat_define_method(Proc, "call", Proc_call);
    nat_define_method(Proc, "lambda?", Proc_lambda);

    NatObject *Exception = nat_subclass(env, Object, "Exception");
    nat_const_set(env, Object, "Exception", Exception);
    nat_define_method(Exception, "initialize", Exception_initialize);
    nat_define_method(Exception, "inspect", Exception_inspect);
    nat_define_method(Exception, "message", Exception_message);
    nat_define_method(Exception, "backtrace", Exception_backtrace);
    nat_define_singleton_method(env, Exception, "new", Exception_new);
    NatObject *StandardError = nat_subclass(env, Exception, "StandardError");
    nat_const_set(env, Object, "StandardError", StandardError);
    NatObject *NameError = nat_subclass(env, StandardError, "NameError");
    nat_const_set(env, Object, "NameError", NameError);
    NatObject *NoMethodError = nat_subclass(env, NameError, "NoMethodError");
    nat_const_set(env, Object, "NoMethodError", NoMethodError);
    NatObject *ArgumentError = nat_subclass(env, StandardError, "ArgumentError");
    nat_const_set(env, Object, "ArgumentError", ArgumentError);
    NatObject *RuntimeError = nat_subclass(env, StandardError, "RuntimeError");
    nat_const_set(env, Object, "RuntimeError", RuntimeError);
    NatObject *TypeError = nat_subclass(env, StandardError, "TypeError");
    nat_const_set(env, Object, "TypeError", TypeError);
    NatObject *SystemExit = nat_subclass(env, StandardError, "SystemExit");
    nat_const_set(env, Object, "SystemExit", SystemExit);

    nat_global_set(env, "$NAT_at_exit_handlers", nat_array(env));

    return env;
}

/*TOP*/

NatObject *EVAL(NatEnv *env) {
    NatObject *self = nat_new(env, Object, 0, NULL, NULL, NULL);
    self->flags = NAT_FLAG_MAIN_OBJECT;
    nat_define_singleton_method(env, self, "inspect", main_obj_inspect);
    obj_language_exceptions(env, self);
    int run_exit_handlers = TRUE;
    if (!NAT_RESCUE(env)) {
        /*BODY*/
        run_exit_handlers = FALSE;
        nat_run_at_exit_handlers(env);
        return nil; // just in case there's no return value
    } else {
        nat_handle_top_level_exception(env, run_exit_handlers);
        return nil;
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
