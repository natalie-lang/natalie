#include <setjmp.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

using namespace Natalie;

/* end of front matter */

/*OBJ_NAT*/

extern "C" Env *build_top_env() {
    Env *env = new Env { new GlobalEnv };
    env->method_name = strdup("<main>");

    ClassValue *Class = ClassValue::bootstrap_class_class(env);
    define_method(env, Class, "superclass", Class_superclass);

    ClassValue *BasicObject = ClassValue::bootstrap_basic_object(env, Class);
    define_method(env, BasicObject, "!", BasicObject_not);
    define_method(env, BasicObject, "==", BasicObject_eqeq);
    define_method(env, BasicObject, "!=", BasicObject_neq);
    define_method(env, BasicObject, "equal?", Kernel_equal);
    define_method(env, BasicObject, "instance_eval", BasicObject_instance_eval);

    ClassValue *Object = NAT_OBJECT = BasicObject->subclass(env, "Object");
    define_singleton_method(env, Object, "new", Object_new);

    // these must be defined after Object exists
    define_singleton_method(env, Class, "new", Class_new);
    BasicObject->singleton_class = Class->singleton_class;
    Object->const_set(env, "Class", Class);
    Object->const_set(env, "BasicObject", BasicObject);
    Object->const_set(env, "Object", Object);

    ClassValue *Module = Object->subclass(env, "Module");
    Object->const_set(env, "Module", Module);
    Class->superclass = Module;
    NAT_MODULE_INIT(Module);

    ModuleValue *Kernel = new ModuleValue { env, "Kernel" };
    Object->const_set(env, "Kernel", Kernel);
    Object->include(env, Kernel);
    NAT_KERNEL_INIT(Kernel);

    ModuleValue *Comparable = new ModuleValue { env, "Comparable" };
    Object->const_set(env, "Comparable", Comparable);
    NAT_COMPARABLE_INIT(Comparable);

    Value *Symbol = Object->subclass(env, "Symbol");
    Object->const_set(env, "Symbol", Symbol);
    NAT_SYMBOL_INIT(Symbol);

    ClassValue *NilClass = Object->subclass(env, "NilClass");
    undefine_singleton_method(env, NilClass, "new");
    Object->const_set(env, "NilClass", NilClass);
    NAT_NIL_CLASS_INIT(NilClass);

    NAT_NIL = NilValue::instance(env);
    NAT_NIL->singleton_class = NilClass;

    ClassValue *TrueClass = Object->subclass(env, "TrueClass");
    undefine_singleton_method(env, TrueClass, "new");
    Object->const_set(env, "TrueClass", TrueClass);
    NAT_TRUE_CLASS_INIT(TrueClass);

    NAT_TRUE = TrueValue::instance(env);
    NAT_TRUE->singleton_class = TrueClass;

    ClassValue *FalseClass = Object->subclass(env, "FalseClass");
    undefine_singleton_method(env, FalseClass, "new");
    Object->const_set(env, "FalseClass", FalseClass);
    NAT_FALSE_CLASS_INIT(FalseClass);

    NAT_FALSE = FalseValue::instance(env);
    NAT_FALSE->singleton_class = FalseClass;

    ClassValue *Numeric = Object->subclass(env, "Numeric");
    Object->const_set(env, "Numeric", Numeric);
    Numeric->include(env, Comparable);

    Value *Integer = NAT_INTEGER = Numeric->subclass(env, "Integer");
    Object->const_set(env, "Integer", Integer);
    Object->const_set(env, "Fixnum", Integer);
    NAT_INTEGER_INIT(Integer);

    ClassValue *String = Object->subclass(env, "String");
    Object->const_set(env, "String", String);
    NAT_STRING_INIT(String);

    ClassValue *Array = Object->subclass(env, "Array");
    Object->const_set(env, "Array", Array);
    NAT_ARRAY_INIT(Array);

    ClassValue *Hash = Object->subclass(env, "Hash");
    Object->const_set(env, "Hash", Hash);
    NAT_HASH_INIT(Hash);

    ClassValue *Regexp = Object->subclass(env, "Regexp");
    Object->const_set(env, "Regexp", Regexp);
    NAT_REGEXP_INIT(Regexp);

    ClassValue *Range = Object->subclass(env, "Range");
    Object->const_set(env, "Range", Range);
    NAT_RANGE_INIT(Range);

    ClassValue *MatchData = Object->subclass(env, "MatchData");
    Object->const_set(env, "MatchData", MatchData);
    NAT_MATCH_DATA_INIT(MatchData);

    ClassValue *Proc = Object->subclass(env, "Proc");
    Object->const_set(env, "Proc", Proc);
    NAT_PROC_INIT(Proc);

    ClassValue *IO = Object->subclass(env, "IO");
    Object->const_set(env, "IO", IO);
    NAT_IO_INIT(IO);

    ClassValue *File = IO->subclass(env, "File");
    Object->const_set(env, "File", File);
    NAT_FILE_INIT(File);

    ClassValue *Exception = Object->subclass(env, "Exception");
    Object->const_set(env, "Exception", Exception);
    define_method(env, Exception, "initialize", Exception_initialize);
    define_method(env, Exception, "inspect", Exception_inspect);
    define_method(env, Exception, "message", Exception_message);
    define_method(env, Exception, "backtrace", Exception_backtrace);
    define_singleton_method(env, Exception, "new", Exception_new);
    ClassValue *ScriptError = Exception->subclass(env, "ScriptError");
    Object->const_set(env, "ScriptError", ScriptError);
    Value *SyntaxError = ScriptError->subclass(env, "SyntaxError");
    Object->const_set(env, "SyntaxError", SyntaxError);
    ClassValue *StandardError = Exception->subclass(env, "StandardError");
    Object->const_set(env, "StandardError", StandardError);
    ClassValue *NameError = StandardError->subclass(env, "NameError");
    Object->const_set(env, "NameError", NameError);
    ClassValue *NoMethodError = NameError->subclass(env, "NoMethodError");
    Object->const_set(env, "NoMethodError", NoMethodError);
    ClassValue *ArgumentError = StandardError->subclass(env, "ArgumentError");
    Object->const_set(env, "ArgumentError", ArgumentError);
    ClassValue *RuntimeError = StandardError->subclass(env, "RuntimeError");
    Object->const_set(env, "RuntimeError", RuntimeError);
    ClassValue *TypeError = StandardError->subclass(env, "TypeError");
    Object->const_set(env, "TypeError", TypeError);
    ClassValue *SystemExit = StandardError->subclass(env, "SystemExit");
    Object->const_set(env, "SystemExit", SystemExit);
    ClassValue *ZeroDivisionError = StandardError->subclass(env, "ZeroDivisionError");
    Object->const_set(env, "ZeroDivisionError", ZeroDivisionError);
    ClassValue *FrozenError = RuntimeError->subclass(env, "FrozenError");
    Object->const_set(env, "FrozenError", FrozenError);

    ClassValue *EncodingError = StandardError->subclass(env, "EncodingError");
    Object->const_set(env, "EncodingError", EncodingError);
    Value *Encoding = NAT_OBJECT->subclass(env, "Encoding");
    Value *InvalidByteSequenceError = EncodingError->subclass(env, "InvalidByteSequenceError");
    Encoding->const_set(env, "InvalidByteSequenceError", InvalidByteSequenceError);
    Value *UndefinedConversionError = EncodingError->subclass(env, "UndefinedConversionError");
    Encoding->const_set(env, "UndefinedConversionError", UndefinedConversionError);
    Value *ConverterNotFoundError = EncodingError->subclass(env, "ConverterNotFoundError");
    Encoding->const_set(env, "ConverterNotFoundError", ConverterNotFoundError);
    Object->const_set(env, "Encoding", Encoding);
    NAT_ENCODING_INIT(Encoding);

    Value *Process = new ModuleValue { env, "Process" };
    Object->const_set(env, "Process", Process);
    NAT_PROCESS_INIT(Process);

    Value *EncodingAscii8Bit = encoding(env, Encoding::ASCII_8BIT, array_with_vals(env, 2, string(env, "ASCII-8BIT"), string(env, "BINARY")));
    Encoding->const_set(env, "ASCII_8BIT", EncodingAscii8Bit);

    Value *EncodingUTF8 = encoding(env, Encoding::UTF_8, array_with_vals(env, 1, string(env, "UTF-8")));
    Encoding->const_set(env, "UTF_8", EncodingUTF8);

    env->global_set("$NAT_at_exit_handlers", array_new(env));

    Value *self = new Value { env };
    self->flags = NAT_FLAG_MAIN_OBJECT;
    define_singleton_method(env, self, "inspect", main_obj_inspect);
    env->global_set("$NAT_main_object", self);

    Value *stdin_fileno = integer(env, STDIN_FILENO);
    Value *stdin = new IoValue { env };
    stdin->initialize(env, 1, &stdin_fileno, nullptr);
    env->global_set("$stdin", stdin);
    Object->const_set(env, "STDIN", stdin);

    Value *stdout_fileno = integer(env, STDOUT_FILENO);
    Value *stdout = new IoValue { env };
    stdout->initialize(env, 1, &stdout_fileno, nullptr);
    env->global_set("$stdout", stdout);
    Object->const_set(env, "STDOUT", stdout);

    Value *stderr_fileno = integer(env, STDERR_FILENO);
    Value *stderr = new IoValue { env };
    stderr->initialize(env, 1, &stderr_fileno, nullptr);
    env->global_set("$stderr", stderr);
    Object->const_set(env, "STDERR", stderr);

    Value *ENV = new Value { env };
    Object->const_set(env, "ENV", ENV);
    NAT_ENV_INIT(ENV);

    Value *RUBY_VERSION = string(env, "2.7.1");
    Object->const_set(env, "RUBY_VERSION", RUBY_VERSION);

    /*OBJ_NAT_INIT*/

    return env;
}

/*TOP*/

extern "C" Value *EVAL(Env *env) {
    Value *self = env->global_get("$NAT_main_object");
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
    Value *ARGV = array_new(env);
    /*INIT*/
    NAT_OBJECT->const_set(env, "ARGV", ARGV);
    assert(argc > 0);
    for (int i = 1; i < argc; i++) {
        array_push(env, ARGV, string(env, argv[i]));
    }
    Value *result = EVAL(env);
    delete env->global_env;
    delete env;
    if (result) {
        return 0;
    } else {
        return 1;
    }
}
