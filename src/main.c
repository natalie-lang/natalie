#include <setjmp.h>

#include "builtin.h"
#include "gc.h"
#include "natalie.h"

/* end of front matter */

/*OBJ_NAT*/

NatEnv *build_top_env() {
    NatEnv *env = malloc(sizeof(NatEnv));
    nat_build_env(env, NULL);
    env->method_name = heap_string("<main>");

    NatObject *Class = nat_alloc(env, NULL, NAT_VALUE_CLASS);
    Class->superclass = NULL;
    Class->class_name = heap_string("Class");
    Class->klass = Class;
    nat_build_env(&Class->env, env);
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, free);
    hashmap_init(&Class->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&Class->constants, hashmap_alloc_key_string, free);
    nat_define_method(env, Class, "superclass", Class_superclass);

    NatObject *BasicObject = nat_alloc(env, Class, NAT_VALUE_CLASS);
    BasicObject->class_name = heap_string("BasicObject");
    nat_build_env(&BasicObject->env, env);
    BasicObject->superclass = NULL;
    BasicObject->cvars.table = NULL;
    hashmap_init(&BasicObject->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&BasicObject->methods, hashmap_alloc_key_string, free);
    hashmap_init(&BasicObject->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&BasicObject->constants, hashmap_alloc_key_string, free);
    nat_define_method(env, BasicObject, "!", BasicObject_not);
    nat_define_method(env, BasicObject, "==", BasicObject_eqeq);
    nat_define_method(env, BasicObject, "!=", BasicObject_neq);
    nat_define_method(env, BasicObject, "equal?", Kernel_equal);
    nat_define_method(env, BasicObject, "instance_eval", BasicObject_instance_eval);

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
    NAT_MODULE_INIT(Module);

    NatObject *Kernel = nat_module(env, "Kernel");
    nat_const_set(env, Object, "Kernel", Kernel);
    nat_class_include(env, Object, Kernel);
    NAT_KERNEL_INIT(Kernel);

    NatObject *Comparable = nat_module(env, "Comparable");
    nat_const_set(env, Object, "Comparable", Comparable);
    NAT_COMPARABLE_INIT(Comparable);

    NatObject *Symbol = nat_subclass(env, Object, "Symbol");
    nat_const_set(env, Object, "Symbol", Symbol);
    NAT_SYMBOL_INIT(Symbol);

    NatObject *NilClass = nat_subclass(env, Object, "NilClass");
    nat_const_set(env, Object, "NilClass", NilClass);
    NAT_NIL_CLASS_INIT(NilClass);

    NAT_NIL = nat_alloc(env, NilClass, NAT_VALUE_NIL);

    NatObject *TrueClass = nat_subclass(env, Object, "TrueClass");
    nat_const_set(env, Object, "TrueClass", TrueClass);
    NAT_TRUE_CLASS_INIT(TrueClass);

    NAT_TRUE = nat_alloc(env, TrueClass, NAT_VALUE_TRUE);

    NatObject *FalseClass = nat_subclass(env, Object, "FalseClass");
    nat_const_set(env, Object, "FalseClass", FalseClass);
    NAT_FALSE_CLASS_INIT(FalseClass);

    NAT_FALSE = nat_alloc(env, FalseClass, NAT_VALUE_FALSE);

    NatObject *Numeric = nat_subclass(env, Object, "Numeric");
    nat_const_set(env, Object, "Numeric", Numeric);
    nat_class_include(env, Numeric, Comparable);

    NatObject *Integer = NAT_INTEGER = nat_subclass(env, Numeric, "Integer");
    nat_const_set(env, Object, "Integer", Integer);
    NAT_INTEGER_INIT(Integer);

    NatObject *String = nat_subclass(env, Object, "String");
    nat_const_set(env, Object, "String", String);
    NAT_STRING_INIT(String);

    NatObject *Array = nat_subclass(env, Object, "Array");
    nat_const_set(env, Object, "Array", Array);
    NAT_ARRAY_INIT(Array);

    NatObject *Hash = nat_subclass(env, Object, "Hash");
    nat_const_set(env, Object, "Hash", Hash);
    NAT_HASH_INIT(Hash);

    NatObject *Regexp = nat_subclass(env, Object, "Regexp");
    nat_const_set(env, Object, "Regexp", Regexp);
    NAT_REGEXP_INIT(Regexp);

    NatObject *Range = nat_subclass(env, Object, "Range");
    nat_const_set(env, Object, "Range", Range);
    NAT_RANGE_INIT(Range);

    NatObject *Thread = nat_subclass(env, Object, "Thread");
    nat_const_set(env, Object, "Thread", Thread);
    NAT_THREAD_INIT(Thread);

    NatObject *MatchData = nat_subclass(env, Object, "MatchData");
    nat_const_set(env, Object, "MatchData", MatchData);
    NAT_MATCH_DATA_INIT(MatchData);

    NatObject *Proc = nat_subclass(env, Object, "Proc");
    nat_const_set(env, Object, "Proc", Proc);
    NAT_PROC_INIT(Proc);

    NatObject *IO = nat_subclass(env, Object, "IO");
    nat_const_set(env, Object, "IO", IO);
    NAT_IO_INIT(IO);

    NatObject *File = nat_subclass(env, IO, "File");
    nat_const_set(env, Object, "File", File);
    NAT_FILE_INIT(File);

    NatObject *Exception = nat_subclass(env, Object, "Exception");
    nat_const_set(env, Object, "Exception", Exception);
    nat_define_method(env, Exception, "initialize", Exception_initialize);
    nat_define_method(env, Exception, "inspect", Exception_inspect);
    nat_define_method(env, Exception, "message", Exception_message);
    nat_define_method(env, Exception, "backtrace", Exception_backtrace);
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

    NatObject *self = nat_alloc(env, NAT_OBJECT, NAT_VALUE_OTHER);
    self->flags = NAT_FLAG_MAIN_OBJECT;
    nat_define_singleton_method(env, self, "inspect", main_obj_inspect);
    nat_global_set(env, "$NAT_main_object", self);

    NatObject *stdin_fileno = nat_integer(env, STDIN_FILENO);
    NatObject *nat_stdin = nat_alloc(env, IO, NAT_VALUE_IO);
    nat_initialize(env, nat_stdin, 1, &stdin_fileno, NULL, NULL);
    nat_global_set(env, "$stdin", nat_stdin);
    nat_const_set(env, Object, "STDIN", nat_stdin);

    NatObject *stdout_fileno = nat_integer(env, STDOUT_FILENO);
    NatObject *nat_stdout = nat_alloc(env, IO, NAT_VALUE_IO);
    nat_initialize(env, nat_stdout, 1, &stdout_fileno, NULL, NULL);
    nat_global_set(env, "$stdout", nat_stdout);
    nat_const_set(env, Object, "STDOUT", nat_stdout);

    NatObject *stderr_fileno = nat_integer(env, STDERR_FILENO);
    NatObject *nat_stderr = nat_alloc(env, IO, NAT_VALUE_IO);
    nat_initialize(env, nat_stderr, 1, &stderr_fileno, NULL, NULL);
    nat_global_set(env, "$stderr", nat_stderr);
    nat_const_set(env, Object, "STDERR", nat_stderr);

    NatObject *ENV = nat_alloc(env, NAT_OBJECT, NAT_VALUE_OTHER);
    nat_const_set(env, Object, "ENV", ENV);
    NAT_ENV_INIT(ENV);

    /*OBJ_NAT_INIT*/

    env->global_env->gc_enabled = true;

    return env;
}

/*TOP*/

NatObject *EVAL(NatEnv *env) {
    NatObject *self = nat_global_get(env, "$NAT_main_object");
    (void)self; // don't warn about unused var
    int run_exit_handlers = true;
    if (!NAT_RESCUE(env)) {
        /*BODY*/
        nat_gc_collect(env);
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
    nat_gc_init(env, &argc);
    NatObject *ARGV = nat_array(env);
    nat_const_set(env, NAT_OBJECT, "ARGV", ARGV);
    assert(argc > 0);
    for (int i = 1; i < argc; i++) {
        nat_array_push(env, ARGV, nat_string(env, argv[i]));
    }
    NatObject *result = EVAL(env);
    nat_gc_collect_all(env);
    if (result) {
        return 0;
    } else {
        return 1;
    }
}
