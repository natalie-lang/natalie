#include <setjmp.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

using namespace Natalie;

/* end of front matter */

/*OBJ_NAT*/

extern "C" Env *build_top_env() {
    Env *env = new Env { new GlobalEnv };
    env->method_name = heap_string("<main>");

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
    const_set(env, Object, "Class", Class);
    const_set(env, Object, "BasicObject", BasicObject);
    const_set(env, Object, "Object", Object);

    ClassValue *Module = Object->subclass(env, "Module");
    const_set(env, Object, "Module", Module);
    Class->superclass = Module;
    NAT_MODULE_INIT(Module);

    ModuleValue *Kernel = new ModuleValue { env, "Kernel" };
    const_set(env, Object, "Kernel", Kernel);
    Object->include(env, Kernel);
    NAT_KERNEL_INIT(Kernel);

    ModuleValue *Comparable = new ModuleValue { env, "Comparable" };
    const_set(env, Object, "Comparable", Comparable);
    NAT_COMPARABLE_INIT(Comparable);

    Value *Symbol = Object->subclass(env, "Symbol");
    const_set(env, Object, "Symbol", Symbol);
    NAT_SYMBOL_INIT(Symbol);

    ClassValue *NilClass = Object->subclass(env, "NilClass");
    undefine_singleton_method(env, NilClass, "new");
    const_set(env, Object, "NilClass", NilClass);
    NAT_NIL_CLASS_INIT(NilClass);

    NAT_NIL = NilValue::instance(env);
    NAT_NIL->singleton_class = NilClass;

    ClassValue *TrueClass = Object->subclass(env, "TrueClass");
    undefine_singleton_method(env, TrueClass, "new");
    const_set(env, Object, "TrueClass", TrueClass);
    NAT_TRUE_CLASS_INIT(TrueClass);

    NAT_TRUE = TrueValue::instance(env);
    NAT_TRUE->singleton_class = TrueClass;

    ClassValue *FalseClass = Object->subclass(env, "FalseClass");
    undefine_singleton_method(env, FalseClass, "new");
    const_set(env, Object, "FalseClass", FalseClass);
    NAT_FALSE_CLASS_INIT(FalseClass);

    NAT_FALSE = FalseValue::instance(env);
    NAT_FALSE->singleton_class = FalseClass;

    ClassValue *Numeric = Object->subclass(env, "Numeric");
    const_set(env, Object, "Numeric", Numeric);
    Numeric->include(env, Comparable);

    Value *Integer = NAT_INTEGER = Numeric->subclass(env, "Integer");
    const_set(env, Object, "Integer", Integer);
    const_set(env, Object, "Fixnum", Integer);
    NAT_INTEGER_INIT(Integer);

    ClassValue *String = Object->subclass(env, "String");
    const_set(env, Object, "String", String);
    NAT_STRING_INIT(String);

    ClassValue *Array = Object->subclass(env, "Array");
    const_set(env, Object, "Array", Array);
    NAT_ARRAY_INIT(Array);

    ClassValue *Hash = Object->subclass(env, "Hash");
    const_set(env, Object, "Hash", Hash);
    NAT_HASH_INIT(Hash);

    ClassValue *Regexp = Object->subclass(env, "Regexp");
    const_set(env, Object, "Regexp", Regexp);
    NAT_REGEXP_INIT(Regexp);

    ClassValue *Range = Object->subclass(env, "Range");
    const_set(env, Object, "Range", Range);
    NAT_RANGE_INIT(Range);

    ClassValue *MatchData = Object->subclass(env, "MatchData");
    const_set(env, Object, "MatchData", MatchData);
    NAT_MATCH_DATA_INIT(MatchData);

    ClassValue *Proc = Object->subclass(env, "Proc");
    const_set(env, Object, "Proc", Proc);
    NAT_PROC_INIT(Proc);

    ClassValue *IO = Object->subclass(env, "IO");
    const_set(env, Object, "IO", IO);
    NAT_IO_INIT(IO);

    ClassValue *File = IO->subclass(env, "File");
    const_set(env, Object, "File", File);
    NAT_FILE_INIT(File);

    ClassValue *Exception = Object->subclass(env, "Exception");
    const_set(env, Object, "Exception", Exception);
    define_method(env, Exception, "initialize", Exception_initialize);
    define_method(env, Exception, "inspect", Exception_inspect);
    define_method(env, Exception, "message", Exception_message);
    define_method(env, Exception, "backtrace", Exception_backtrace);
    define_singleton_method(env, Exception, "new", Exception_new);
    ClassValue *ScriptError = Exception->subclass(env, "ScriptError");
    const_set(env, Object, "ScriptError", ScriptError);
    Value *SyntaxError = ScriptError->subclass(env, "SyntaxError");
    const_set(env, Object, "SyntaxError", SyntaxError);
    ClassValue *StandardError = Exception->subclass(env, "StandardError");
    const_set(env, Object, "StandardError", StandardError);
    ClassValue *NameError = StandardError->subclass(env, "NameError");
    const_set(env, Object, "NameError", NameError);
    ClassValue *NoMethodError = NameError->subclass(env, "NoMethodError");
    const_set(env, Object, "NoMethodError", NoMethodError);
    ClassValue *ArgumentError = StandardError->subclass(env, "ArgumentError");
    const_set(env, Object, "ArgumentError", ArgumentError);
    ClassValue *RuntimeError = StandardError->subclass(env, "RuntimeError");
    const_set(env, Object, "RuntimeError", RuntimeError);
    ClassValue *TypeError = StandardError->subclass(env, "TypeError");
    const_set(env, Object, "TypeError", TypeError);
    ClassValue *SystemExit = StandardError->subclass(env, "SystemExit");
    const_set(env, Object, "SystemExit", SystemExit);
    ClassValue *ZeroDivisionError = StandardError->subclass(env, "ZeroDivisionError");
    const_set(env, Object, "ZeroDivisionError", ZeroDivisionError);
    ClassValue *FrozenError = RuntimeError->subclass(env, "FrozenError");
    const_set(env, Object, "FrozenError", FrozenError);

    ClassValue *EncodingError = StandardError->subclass(env, "EncodingError");
    const_set(env, Object, "EncodingError", EncodingError);
    Value *Encoding = NAT_OBJECT->subclass(env, "Encoding");
    Value *InvalidByteSequenceError = EncodingError->subclass(env, "InvalidByteSequenceError");
    const_set(env, Encoding, "InvalidByteSequenceError", InvalidByteSequenceError);
    Value *UndefinedConversionError = EncodingError->subclass(env, "UndefinedConversionError");
    const_set(env, Encoding, "UndefinedConversionError", UndefinedConversionError);
    Value *ConverterNotFoundError = EncodingError->subclass(env, "ConverterNotFoundError");
    const_set(env, Encoding, "ConverterNotFoundError", ConverterNotFoundError);
    const_set(env, Object, "Encoding", Encoding);
    NAT_ENCODING_INIT(Encoding);

    Value *Process = new ModuleValue { env, "Process" };
    const_set(env, Object, "Process", Process);
    NAT_PROCESS_INIT(Process);

    Value *EncodingAscii8Bit = encoding(env, Encoding::ASCII_8BIT, array_with_vals(env, 2, string(env, "ASCII-8BIT"), string(env, "BINARY")));
    const_set(env, Encoding, "ASCII_8BIT", EncodingAscii8Bit);

    Value *EncodingUTF8 = encoding(env, Encoding::UTF_8, array_with_vals(env, 1, string(env, "UTF-8")));
    const_set(env, Encoding, "UTF_8", EncodingUTF8);

    global_set(env, "$NAT_at_exit_handlers", array_new(env));

    Value *self = new Value { env };
    self->flags = NAT_FLAG_MAIN_OBJECT;
    define_singleton_method(env, self, "inspect", main_obj_inspect);
    global_set(env, "$NAT_main_object", self);

    Value *stdin_fileno = integer(env, STDIN_FILENO);
    Value *stdin = new IoValue { env };
    stdin->initialize(env, 1, &stdin_fileno, nullptr);
    global_set(env, "$stdin", stdin);
    const_set(env, Object, "STDIN", stdin);

    Value *stdout_fileno = integer(env, STDOUT_FILENO);
    Value *stdout = new IoValue { env };
    stdout->initialize(env, 1, &stdout_fileno, nullptr);
    global_set(env, "$stdout", stdout);
    const_set(env, Object, "STDOUT", stdout);

    Value *stderr_fileno = integer(env, STDERR_FILENO);
    Value *stderr = new IoValue { env };
    stderr->initialize(env, 1, &stderr_fileno, nullptr);
    global_set(env, "$stderr", stderr);
    const_set(env, Object, "STDERR", stderr);

    Value *ENV = new Value { env };
    const_set(env, Object, "ENV", ENV);
    NAT_ENV_INIT(ENV);

    Value *RUBY_VERSION = string(env, "2.7.1");
    const_set(env, Object, "RUBY_VERSION", RUBY_VERSION);

    /*OBJ_NAT_INIT*/

    return env;
}

/*TOP*/

extern "C" Value *EVAL(Env *env) {
    Value *self = global_get(env, "$NAT_main_object");
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
    const_set(env, NAT_OBJECT, "ARGV", ARGV);
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
