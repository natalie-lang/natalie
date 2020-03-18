#include <setjmp.h>

#include "natalie.h"
#include "builtin.h"

/* end of front matter */

/*OBJ_NAT*/

NatEnv *build_top_env() {
    NatEnv *env = malloc(sizeof(NatEnv));
    nat_build_env(env, NULL);
    env->method_name = heap_string("<main>");

    NatObject *Class = nat_alloc(env);
    Class->type = NAT_VALUE_CLASS;
    Class->class_name = heap_string("Class");
    Class->klass = Class;
    nat_build_env(&Class->env, env);
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, NULL);
    hashmap_init(&Class->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&Class->constants, hashmap_alloc_key_string, NULL);
    nat_define_method(Class, "superclass", Class_superclass);

    NatObject *BasicObject = nat_alloc(env);
    BasicObject->type = NAT_VALUE_CLASS;
    BasicObject->class_name = heap_string("BasicObject");
    BasicObject->klass = Class;
    nat_build_env(&BasicObject->env, env);
    BasicObject->superclass = NULL;
    BasicObject->cvars.table = NULL;
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

    NatObject *Object = NAT_OBJECT = nat_subclass(env, BasicObject, "Object");
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
    nat_define_method(Kernel, "lambda", Kernel_lambda);
    nat_define_method(Kernel, "__method__", Kernel_method);
    nat_define_method(Kernel, "freeze", Kernel_freeze);
    nat_define_method(Kernel, "nil?", Kernel_is_nil);

    NatObject *Comparable = nat_module(env, "Comparable");
    nat_const_set(env, Object, "Comparable", Comparable);
    COMPARABLE_INIT();

    NatObject *Symbol = nat_subclass(env, Object, "Symbol");
    nat_const_set(env, Object, "Symbol", Symbol);
    nat_define_method(Symbol, "to_s", Symbol_to_s);
    nat_define_method(Symbol, "inspect", Symbol_inspect);

    NatObject *NilClass = nat_subclass(env, Object, "NilClass");
    nat_const_set(env, Object, "NilClass", NilClass);
    nat_undefine_singleton_method(env, NilClass, "new");
    nat_define_method(NilClass, "to_s", NilClass_to_s);
    nat_define_method(NilClass, "to_a", NilClass_to_a);
    nat_define_method(NilClass, "inspect", NilClass_inspect);
    nat_define_method(NilClass, "nil?", NilClass_is_nil);

    NatObject *nil = NAT_NIL = nat_new(env, NilClass, 0, NULL, NULL, NULL);
    nil->type = NAT_VALUE_NIL;

    NatObject *TrueClass = nat_subclass(env, Object, "TrueClass");
    nat_const_set(env, Object, "TrueClass", TrueClass);
    nat_undefine_singleton_method(env, TrueClass, "new");
    nat_define_method(TrueClass, "to_s", TrueClass_to_s);
    nat_define_method(TrueClass, "inspect", TrueClass_to_s);

    NatObject *true_obj = NAT_TRUE = nat_new(env, TrueClass, 0, NULL, NULL, NULL);
    true_obj->type = NAT_VALUE_TRUE;

    NatObject *FalseClass = nat_subclass(env, Object, "FalseClass");
    nat_const_set(env, Object, "FalseClass", FalseClass);
    nat_undefine_singleton_method(env, FalseClass, "new");
    nat_define_method(FalseClass, "to_s", FalseClass_to_s);
    nat_define_method(FalseClass, "inspect", FalseClass_to_s);

    NatObject *false_obj = NAT_FALSE = nat_new(env, FalseClass, 0, NULL, NULL, NULL);
    false_obj->type = NAT_VALUE_FALSE;

    NatObject *Numeric = nat_subclass(env, Object, "Numeric");
    nat_const_set(env, Object, "Numeric", Numeric);
    nat_class_include(Numeric, Comparable);

    NatObject *Integer = NAT_INTEGER = nat_subclass(env, Numeric, "Integer");
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
    nat_define_method(Integer, "&", Integer_bitwise_and);
    nat_define_method(Integer, "|", Integer_bitwise_or);
    nat_define_method(Integer, "succ", Integer_succ);

    NatObject *String = nat_subclass(env, Object, "String");
    nat_const_set(env, Object, "String", String);
    nat_define_singleton_method(env, String, "new", String_new);
    nat_class_include(String, Comparable);
    nat_define_method(String, "to_s", String_to_s);
    nat_define_method(String, "inspect", String_inspect);
    nat_define_method(String, "<=>", String_cmp);
    nat_define_method(String, "<<", String_ltlt);
    nat_define_method(String, "+", String_add);
    nat_define_method(String, "*", String_mul);
    nat_define_method(String, "==", String_eqeq);
    nat_define_method(String, "===", String_eqeq);
    nat_define_method(String, "=~", String_eqtilde);
    nat_define_method(String, "match", String_match);
    nat_define_method(String, "succ", String_succ);

    NatObject *Array = nat_subclass(env, Object, "Array");
    nat_const_set(env, Object, "Array", Array);
    nat_define_method(Array, "inspect", Array_inspect);
    nat_define_method(Array, "to_s", Array_inspect);
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
    nat_define_method(Array, "sort", Array_sort);

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

    NatObject *Regexp = nat_subclass(env, Object, "Regexp");
    nat_const_set(env, Object, "Regexp", Regexp);
    nat_define_singleton_method(env, Regexp, "new", Regexp_new);
    nat_define_method(Regexp, "==", Regexp_eqeq);
    nat_define_method(Regexp, "===", Regexp_match);
    nat_define_method(Regexp, "inspect", Regexp_inspect);
    nat_define_method(Regexp, "=~", Regexp_eqtilde);
    nat_define_method(Regexp, "match", Regexp_match);

    NatObject *Range = nat_subclass(env, Object, "Range");
    nat_const_set(env, Object, "Range", Range);
    nat_define_singleton_method(env, Range, "new", Range_new);
    nat_define_method(Range, "begin", Range_begin);
    nat_define_method(Range, "first", Range_begin);
    nat_define_method(Range, "end", Range_end);
    nat_define_method(Range, "last", Range_end);
    nat_define_method(Range, "exclude_end?", Range_exclude_end);
    nat_define_method(Range, "to_a", Range_to_a);
    nat_define_method(Range, "each", Range_each);
    nat_define_method(Range, "inspect", Range_inspect);

    NatObject *Thread = nat_subclass(env, Object, "Thread");
    nat_const_set(env, Object, "Thread", Thread);
    nat_define_singleton_method(env, Thread, "new", Thread_new);
    nat_define_method(Thread, "join", Thread_join);
    nat_define_method(Thread, "value", Thread_value);

    NatObject *MatchData = nat_subclass(env, Object, "MatchData");
    nat_const_set(env, Object, "MatchData", MatchData);
    nat_define_method(MatchData, "size", MatchData_size);
    nat_define_method(MatchData, "length", MatchData_size);
    nat_define_method(MatchData, "to_s", MatchData_to_s);
    nat_define_method(MatchData, "[]", MatchData_ref);

    NatObject *Proc = nat_subclass(env, Object, "Proc");
    nat_const_set(env, Object, "Proc", Proc);
    nat_define_singleton_method(env, Proc, "new", Proc_new);
    nat_define_method(Proc, "call", Proc_call);
    nat_define_method(Proc, "lambda?", Proc_lambda);

    NatObject *IO = nat_subclass(env, Object, "IO");
    nat_const_set(env, Object, "IO", IO);
    nat_define_singleton_method(env, IO, "new", IO_new);
    nat_define_method(IO, "initialize", IO_initialize);
    nat_define_method(IO, "fileno", IO_fileno);
    nat_define_method(IO, "read", IO_read);
    nat_define_method(IO, "write", IO_write);
    nat_define_method(IO, "puts", IO_puts);
    nat_define_method(IO, "print", IO_print);
    nat_define_method(IO, "close", IO_close);
    nat_define_method(IO, "seek", IO_seek);
    NatObject *File = nat_subclass(env, IO, "File");
    nat_const_set(env, Object, "File", File);
    nat_define_method(File, "initialize", File_initialize);
    FILE_INIT();

    NatObject *Exception = nat_subclass(env, Object, "Exception");
    nat_const_set(env, Object, "Exception", Exception);
    nat_define_method(Exception, "initialize", Exception_initialize);
    nat_define_method(Exception, "inspect", Exception_inspect);
    nat_define_method(Exception, "message", Exception_message);
    nat_define_method(Exception, "backtrace", Exception_backtrace);
    nat_define_singleton_method(env, Exception, "new", Exception_new);
    NatObject *ScriptError = nat_subclass(env, Exception, "ScriptError");
    nat_const_set(env, Object, "ScriptError", ScriptError);
    NatObject *SyntaxError = nat_subclass(env, ScriptError, "SyntaxError");
    nat_const_set(env, Object, "SyntaxError", SyntaxError);
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
    NatObject *ZeroDivisionError = nat_subclass(env, StandardError, "ZeroDivisionError");
    nat_const_set(env, Object, "ZeroDivisionError", ZeroDivisionError);
    NatObject *ThreadError = nat_subclass(env, StandardError, "ThreadError");
    nat_const_set(env, Object, "ThreadError", ThreadError);
    NatObject *FrozenError = nat_subclass(env, RuntimeError, "FrozenError");
    nat_const_set(env, Object, "FrozenError", FrozenError);

    nat_global_set(env, "$NAT_at_exit_handlers", nat_array(env));

    NatObject *self = nat_new(env, NAT_OBJECT, 0, NULL, NULL, NULL);
    self->flags = NAT_FLAG_MAIN_OBJECT;
    nat_define_singleton_method(env, self, "inspect", main_obj_inspect);
    nat_global_set(env, "$NAT_main_object", self);

    NatObject *stdin_fileno = nat_integer(env, STDIN_FILENO);
    NatObject *nat_stdin = nat_new(env, IO, 1, &stdin_fileno, NULL, NULL);
    nat_global_set(env, "$stdin", nat_stdin);
    nat_const_set(env, Object, "STDIN", nat_stdin);

    NatObject *stdout_fileno = nat_integer(env, STDOUT_FILENO);
    NatObject *nat_stdout = nat_new(env, IO, 1, &stdout_fileno, NULL, NULL);
    nat_global_set(env, "$stdout", nat_stdout);
    nat_const_set(env, Object, "STDOUT", nat_stdout);

    NatObject *stderr_fileno = nat_integer(env, STDERR_FILENO);
    NatObject *nat_stderr = nat_new(env, IO, 1, &stderr_fileno, NULL, NULL);
    nat_global_set(env, "$stderr", nat_stderr);
    nat_const_set(env, Object, "STDERR", nat_stderr);

    /*OBJ_NAT_INIT*/

    return env;
}

/*TOP*/

NatObject *EVAL(NatEnv *env) {
    NatObject *self = nat_global_get(env, "$NAT_main_object");
    (void)self; // don't warn about unused var
    int run_exit_handlers = true;
    if (!NAT_RESCUE(env)) {
        /*BODY*/
        run_exit_handlers = false;
        nat_run_at_exit_handlers(env);
        return NAT_NIL; // just in case there's no return value
    } else {
        nat_handle_top_level_exception(env, run_exit_handlers);
        return NAT_NIL;
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 1024);
    NatEnv *env = build_top_env();
    NatObject *ARGV = nat_array(env);
    nat_const_set(env, NAT_OBJECT, "ARGV", ARGV);
    assert(argc > 0);
    for (int i=1; i<argc; i++) {
        nat_array_push(ARGV, nat_string(env, argv[i]));
    }
    if (EVAL(env)) {
        return 0;
    } else {
        return 1;
    }
}
