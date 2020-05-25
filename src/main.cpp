#include <setjmp.h>

#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"

using namespace Natalie;

/* end of front matter */

/*OBJ_NAT*/

extern "C" Env *build_top_env() {
    Env *env = static_cast<Env *>(malloc(sizeof(Env)));
    build_env(env, NULL);
    env->method_name = heap_string("<main>");

    NatObject *Class = alloc(env, NULL, NAT_VALUE_CLASS);
    Class->superclass = NULL;
    Class->class_name = heap_string("Class");
    Class->klass = Class;
    build_env(&Class->env, env);
    hashmap_init(&Class->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&Class->methods, hashmap_alloc_key_string, free);
    hashmap_init(&Class->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&Class->constants, hashmap_alloc_key_string, free);
    define_method(env, Class, "superclass", Class_superclass);

    NatObject *BasicObject = alloc(env, Class, NAT_VALUE_CLASS);
    BasicObject->class_name = heap_string("BasicObject");
    build_env(&BasicObject->env, env);
    BasicObject->superclass = NULL;
    BasicObject->cvars.table = NULL;
    hashmap_init(&BasicObject->methods, hashmap_hash_string, hashmap_compare_string, 100);
    hashmap_set_key_alloc_funcs(&BasicObject->methods, hashmap_alloc_key_string, free);
    hashmap_init(&BasicObject->constants, hashmap_hash_string, hashmap_compare_string, 10);
    hashmap_set_key_alloc_funcs(&BasicObject->constants, hashmap_alloc_key_string, free);
    define_method(env, BasicObject, "!", BasicObject_not);
    define_method(env, BasicObject, "==", BasicObject_eqeq);
    define_method(env, BasicObject, "!=", BasicObject_neq);
    define_method(env, BasicObject, "equal?", Kernel_equal);
    define_method(env, BasicObject, "instance_eval", BasicObject_instance_eval);

    NatObject *Object = NAT_OBJECT = subclass(env, BasicObject, "Object");
    define_singleton_method(env, Object, "new", Object_new);

    // these must be defined after Object exists
    define_singleton_method(env, Class, "new", Class_new);
    BasicObject->singleton_class = Class->singleton_class;
    const_set(env, Object, "Class", Class);
    const_set(env, Object, "BasicObject", BasicObject);
    const_set(env, Object, "Object", Object);

    NatObject *Module = subclass(env, Object, "Module");
    const_set(env, Object, "Module", Module);
    Class->superclass = Module;
    NAT_MODULE_INIT(Module);

    NatObject *Kernel = module(env, "Kernel");
    const_set(env, Object, "Kernel", Kernel);
    class_include(env, Object, Kernel);
    NAT_KERNEL_INIT(Kernel);

    NatObject *Comparable = module(env, "Comparable");
    const_set(env, Object, "Comparable", Comparable);
    NAT_COMPARABLE_INIT(Comparable);

    NatObject *Symbol = subclass(env, Object, "Symbol");
    const_set(env, Object, "Symbol", Symbol);
    NAT_SYMBOL_INIT(Symbol);

    NatObject *NilClass = subclass(env, Object, "NilClass");
    undefine_singleton_method(env, NilClass, "new");
    const_set(env, Object, "NilClass", NilClass);
    NAT_NIL_CLASS_INIT(NilClass);

    NAT_NIL = alloc(env, NilClass, NAT_VALUE_NIL);
    NAT_NIL->singleton_class = NilClass;

    NatObject *TrueClass = subclass(env, Object, "TrueClass");
    undefine_singleton_method(env, TrueClass, "new");
    const_set(env, Object, "TrueClass", TrueClass);
    NAT_TRUE_CLASS_INIT(TrueClass);

    NAT_TRUE = alloc(env, TrueClass, NAT_VALUE_TRUE);
    NAT_TRUE->singleton_class = TrueClass;

    NatObject *FalseClass = subclass(env, Object, "FalseClass");
    undefine_singleton_method(env, FalseClass, "new");
    const_set(env, Object, "FalseClass", FalseClass);
    NAT_FALSE_CLASS_INIT(FalseClass);

    NAT_FALSE = alloc(env, FalseClass, NAT_VALUE_FALSE);
    NAT_FALSE->singleton_class = FalseClass;

    NatObject *Numeric = subclass(env, Object, "Numeric");
    const_set(env, Object, "Numeric", Numeric);
    class_include(env, Numeric, Comparable);

    NatObject *Integer = NAT_INTEGER = subclass(env, Numeric, "Integer");
    const_set(env, Object, "Integer", Integer);
    const_set(env, Object, "Fixnum", Integer);
    NAT_INTEGER_INIT(Integer);

    NatObject *String = subclass(env, Object, "String");
    const_set(env, Object, "String", String);
    NAT_STRING_INIT(String);

    NatObject *Array = subclass(env, Object, "Array");
    const_set(env, Object, "Array", Array);
    NAT_ARRAY_INIT(Array);

    NatObject *Hash = subclass(env, Object, "Hash");
    const_set(env, Object, "Hash", Hash);
    NAT_HASH_INIT(Hash);

    NatObject *Regexp = subclass(env, Object, "Regexp");
    const_set(env, Object, "Regexp", Regexp);
    NAT_REGEXP_INIT(Regexp);

    NatObject *Range = subclass(env, Object, "Range");
    const_set(env, Object, "Range", Range);
    NAT_RANGE_INIT(Range);

    NatObject *MatchData = subclass(env, Object, "MatchData");
    const_set(env, Object, "MatchData", MatchData);
    NAT_MATCH_DATA_INIT(MatchData);

    NatObject *Proc = subclass(env, Object, "Proc");
    const_set(env, Object, "Proc", Proc);
    NAT_PROC_INIT(Proc);

    NatObject *IO = subclass(env, Object, "IO");
    const_set(env, Object, "IO", IO);
    NAT_IO_INIT(IO);

    NatObject *File = subclass(env, IO, "File");
    const_set(env, Object, "File", File);
    NAT_FILE_INIT(File);

    NatObject *Exception = subclass(env, Object, "Exception");
    const_set(env, Object, "Exception", Exception);
    define_method(env, Exception, "initialize", Exception_initialize);
    define_method(env, Exception, "inspect", Exception_inspect);
    define_method(env, Exception, "message", Exception_message);
    define_method(env, Exception, "backtrace", Exception_backtrace);
    define_singleton_method(env, Exception, "new", Exception_new);
    NatObject *ScriptError = subclass(env, Exception, "ScriptError");
    const_set(env, Object, "ScriptError", ScriptError);
    NatObject *SyntaxError = subclass(env, ScriptError, "SyntaxError");
    const_set(env, Object, "SyntaxError", SyntaxError);
    NatObject *StandardError = subclass(env, Exception, "StandardError");
    const_set(env, Object, "StandardError", StandardError);
    NatObject *NameError = subclass(env, StandardError, "NameError");
    const_set(env, Object, "NameError", NameError);
    NatObject *NoMethodError = subclass(env, NameError, "NoMethodError");
    const_set(env, Object, "NoMethodError", NoMethodError);
    NatObject *ArgumentError = subclass(env, StandardError, "ArgumentError");
    const_set(env, Object, "ArgumentError", ArgumentError);
    NatObject *RuntimeError = subclass(env, StandardError, "RuntimeError");
    const_set(env, Object, "RuntimeError", RuntimeError);
    NatObject *TypeError = subclass(env, StandardError, "TypeError");
    const_set(env, Object, "TypeError", TypeError);
    NatObject *SystemExit = subclass(env, StandardError, "SystemExit");
    const_set(env, Object, "SystemExit", SystemExit);
    NatObject *ZeroDivisionError = subclass(env, StandardError, "ZeroDivisionError");
    const_set(env, Object, "ZeroDivisionError", ZeroDivisionError);
    NatObject *FrozenError = subclass(env, RuntimeError, "FrozenError");
    const_set(env, Object, "FrozenError", FrozenError);

    NatObject *EncodingError = subclass(env, StandardError, "EncodingError");
    const_set(env, Object, "EncodingError", EncodingError);
    NatObject *Encoding = subclass(env, NAT_OBJECT, "Encoding");
    NatObject *InvalidByteSequenceError = subclass(env, EncodingError, "InvalidByteSequenceError");
    const_set(env, Encoding, "InvalidByteSequenceError", InvalidByteSequenceError);
    NatObject *UndefinedConversionError = subclass(env, EncodingError, "UndefinedConversionError");
    const_set(env, Encoding, "UndefinedConversionError", UndefinedConversionError);
    NatObject *ConverterNotFoundError = subclass(env, EncodingError, "ConverterNotFoundError");
    const_set(env, Encoding, "ConverterNotFoundError", ConverterNotFoundError);
    const_set(env, Object, "Encoding", Encoding);
    NAT_ENCODING_INIT(Encoding);

    NatObject *Process = module(env, "Process");
    const_set(env, Object, "Process", Process);
    NAT_PROCESS_INIT(Process);

    NatObject *EncodingAscii8Bit = encoding(env, NAT_ENCODING_ASCII_8BIT, array_with_vals(env, 2, string(env, "ASCII-8BIT"), string(env, "BINARY")));
    const_set(env, Encoding, "ASCII_8BIT", EncodingAscii8Bit);

    NatObject *EncodingUTF8 = encoding(env, NAT_ENCODING_UTF_8, array_with_vals(env, 1, string(env, "UTF-8")));
    const_set(env, Encoding, "UTF_8", EncodingUTF8);

    global_set(env, "$NAT_at_exit_handlers", array_new(env));

    NatObject *self = alloc(env, NAT_OBJECT, NAT_VALUE_OTHER);
    self->flags = NAT_FLAG_MAIN_OBJECT;
    define_singleton_method(env, self, "inspect", main_obj_inspect);
    global_set(env, "$NAT_main_object", self);

    NatObject *stdin_fileno = integer(env, STDIN_FILENO);
    NatObject *stdin = alloc(env, IO, NAT_VALUE_IO);
    initialize(env, stdin, 1, &stdin_fileno, NULL);
    global_set(env, "$stdin", stdin);
    const_set(env, Object, "STDIN", stdin);

    NatObject *stdout_fileno = integer(env, STDOUT_FILENO);
    NatObject *stdout = alloc(env, IO, NAT_VALUE_IO);
    initialize(env, stdout, 1, &stdout_fileno, NULL);
    global_set(env, "$stdout", stdout);
    const_set(env, Object, "STDOUT", stdout);

    NatObject *stderr_fileno = integer(env, STDERR_FILENO);
    NatObject *stderr = alloc(env, IO, NAT_VALUE_IO);
    initialize(env, stderr, 1, &stderr_fileno, NULL);
    global_set(env, "$stderr", stderr);
    const_set(env, Object, "STDERR", stderr);

    NatObject *ENV = alloc(env, NAT_OBJECT, NAT_VALUE_OTHER);
    const_set(env, Object, "ENV", ENV);
    NAT_ENV_INIT(ENV);

    NatObject *RUBY_VERSION = string(env, "2.7.1");
    const_set(env, Object, "RUBY_VERSION", RUBY_VERSION);

    /*OBJ_NAT_INIT*/

    return env;
}

/*TOP*/

extern "C" NatObject *EVAL(Env *env) {
    NatObject *self = global_get(env, "$NAT_main_object");
    (void)self; // don't warn about unused var
    volatile bool run_exit_handlers = true;
    if (!NAT_RESCUE(env)) {
        /*BODY*/
        run_exit_handlers = false;
        run_at_exit_handlers(env);
        return NAT_NIL; // just in case there's no return value
    } else {
        handle_top_level_exception(env, run_exit_handlers);
        return NAT_NIL;
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 1024);
    Env *env = build_top_env();
    NatObject *ARGV = array_new(env);
    /*INIT*/
    const_set(env, NAT_OBJECT, "ARGV", ARGV);
    assert(argc > 0);
    for (int i = 1; i < argc; i++) {
        array_push(env, ARGV, string(env, argv[i]));
    }
    gc_init(env, &argc);
    NatObject *result = EVAL(env);
    gc_collect_all(env);
    free_global_env(env->global_env);
    free(env);
    if (result) {
        return 0;
    } else {
        return 1;
    }
}
