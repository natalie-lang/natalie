#include "natalie.hpp"
#include "natalie/builtin.hpp"

using namespace Natalie;

/* end of front matter */

/*OBJ_NAT*/

extern "C" Env *build_top_env() {
    Env *env = new Env { new GlobalEnv };
    env->set_method_name(strdup("<main>"));

    ClassValue *Class = ClassValue::bootstrap_class_class(env);

    ClassValue *BasicObject = ClassValue::bootstrap_basic_object(env, Class);
    BasicObject->define_method(env, "!", BasicObject_not);
    BasicObject->define_method(env, "==", BasicObject_eqeq);
    BasicObject->define_method(env, "!=", BasicObject_neq);
    BasicObject->define_method(env, "equal?", Kernel_equal);
    BasicObject->define_method(env, "instance_eval", BasicObject_instance_eval);

    ClassValue *Object = BasicObject->subclass(env, "Object");
    env->global_env()->set_Object(Object);
    Object->define_singleton_method(env, "new", Object_new);

    // these must be defined after Object exists
    BasicObject->set_singleton_class(Class->singleton_class(env));
    Object->const_set(env, "Class", Class);
    Object->const_set(env, "BasicObject", BasicObject);
    Object->const_set(env, "Object", Object);

    ClassValue *Module = Object->subclass(env, "Module", Value::Type::Module);
    Object->const_set(env, "Module", Module);
    Class->set_superclass_DANGEROUSLY(Module);
    NAT_MODULE_INIT(Module);

    ModuleValue *Kernel = new ModuleValue { env, "Kernel" };
    Object->const_set(env, "Kernel", Kernel);
    Object->include(env, Kernel);
    NAT_KERNEL_INIT(Kernel);

    ModuleValue *Comparable = new ModuleValue { env, "Comparable" };
    Object->const_set(env, "Comparable", Comparable);
    NAT_COMPARABLE_INIT(Comparable);

    ClassValue *Symbol = Object->subclass(env, "Symbol", Value::Type::Symbol);
    Object->const_set(env, "Symbol", Symbol);
    NAT_SYMBOL_INIT(Symbol);

    ClassValue *NilClass = Object->subclass(env, "NilClass", Value::Type::Nil);
    Object->const_set(env, "NilClass", NilClass);

    env->global_env()->set_nil(NilValue::instance(env));
    NAT_NIL->set_singleton_class(NilClass);

    ClassValue *TrueClass = Object->subclass(env, "TrueClass", Value::Type::True);
    Object->const_set(env, "TrueClass", TrueClass);

    env->global_env()->set_true_obj(TrueValue::instance(env));
    NAT_TRUE->set_singleton_class(TrueClass);

    ClassValue *FalseClass = Object->subclass(env, "FalseClass", Value::Type::False);
    Object->const_set(env, "FalseClass", FalseClass);

    env->global_env()->set_false_obj(FalseValue::instance(env));
    NAT_FALSE->set_singleton_class(FalseClass);

    ClassValue *Numeric = Object->subclass(env, "Numeric");
    Object->const_set(env, "Numeric", Numeric);
    Numeric->include(env, Comparable);

    ClassValue *Integer = Numeric->subclass(env, "Integer", Value::Type::Integer);
    Object->const_set(env, "Integer", Integer);
    Object->const_set(env, "Fixnum", Integer);

    ClassValue *Float = Numeric->subclass(env, "Float", Value::Type::Float);
    Object->const_set(env, "Float", Float);
    Float->const_set(env, "MIN", new FloatValue { env, NAT_MIN_FLOAT });
    Float->const_set(env, "MAX", new FloatValue { env, NAT_MAX_FLOAT });

    ClassValue *String = Object->subclass(env, "String", Value::Type::String);
    Object->const_set(env, "String", String);
    String->include(env, Comparable);

    ClassValue *Array = Object->subclass(env, "Array", Value::Type::Array);
    Object->const_set(env, "Array", Array);

    ClassValue *Hash = Object->subclass(env, "Hash", Value::Type::Hash);
    Object->const_set(env, "Hash", Hash);
    NAT_HASH_INIT(Hash);

    ClassValue *Regexp = Object->subclass(env, "Regexp", Value::Type::Regexp);
    Object->const_set(env, "Regexp", Regexp);

    ClassValue *Range = Object->subclass(env, "Range", Value::Type::Range);
    Object->const_set(env, "Range", Range);
    NAT_RANGE_INIT(Range);

    ClassValue *MatchData = Object->subclass(env, "MatchData", Value::Type::MatchData);
    Object->const_set(env, "MatchData", MatchData);
    NAT_MATCH_DATA_INIT(MatchData);

    ClassValue *Proc = Object->subclass(env, "Proc", Value::Type::Proc);
    Object->const_set(env, "Proc", Proc);
    NAT_PROC_INIT(Proc);

    ClassValue *IO = Object->subclass(env, "IO", Value::Type::Io);
    Object->const_set(env, "IO", IO);
    NAT_IO_INIT(IO);

    ClassValue *File = IO->subclass(env, "File");
    Object->const_set(env, "File", File);
    NAT_FILE_INIT(File);

    ClassValue *Exception = Object->subclass(env, "Exception", Value::Type::Exception);
    Object->const_set(env, "Exception", Exception);
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
    ClassValue *ZeroDivisionError = StandardError->subclass(env, "ZeroDivisionError");
    Object->const_set(env, "ZeroDivisionError", ZeroDivisionError);
    ClassValue *FrozenError = RuntimeError->subclass(env, "FrozenError");
    Object->const_set(env, "FrozenError", FrozenError);
    ClassValue *RangeError = StandardError->subclass(env, "RangeError");
    Object->const_set(env, "RangeError", RangeError);
    ClassValue *FloatDomainError = RangeError->subclass(env, "FloatDomainError");
    Object->const_set(env, "FloatDomainError", FloatDomainError);
    ClassValue *IndexError = StandardError->subclass(env, "IndexError");
    Object->const_set(env, "IndexError", IndexError);

    ClassValue *EncodingError = StandardError->subclass(env, "EncodingError");
    Object->const_set(env, "EncodingError", EncodingError);
    ClassValue *Encoding = NAT_OBJECT->subclass(env, "Encoding");
    Value *InvalidByteSequenceError = EncodingError->subclass(env, "InvalidByteSequenceError");
    Encoding->const_set(env, "InvalidByteSequenceError", InvalidByteSequenceError);
    Value *UndefinedConversionError = EncodingError->subclass(env, "UndefinedConversionError");
    Encoding->const_set(env, "UndefinedConversionError", UndefinedConversionError);
    Value *ConverterNotFoundError = EncodingError->subclass(env, "ConverterNotFoundError");
    Encoding->const_set(env, "ConverterNotFoundError", ConverterNotFoundError);
    Object->const_set(env, "Encoding", Encoding);

    Value *Process = new ModuleValue { env, "Process" };
    Object->const_set(env, "Process", Process);
    NAT_PROCESS_INIT(Process);

    EncodingValue *EncodingAscii8Bit = new EncodingValue { env, Encoding::ASCII_8BIT, std::initializer_list<const char *>({ "ASCII-8BIT", "BINARY" }) };
    Encoding->const_set(env, "ASCII_8BIT", EncodingAscii8Bit);

    Value *EncodingUTF8 = new EncodingValue { env, Encoding::UTF_8, std::initializer_list<const char *>({ "UTF-8" }) };
    Encoding->const_set(env, "UTF_8", EncodingUTF8);

    env->global_set("$NAT_at_exit_handlers", new ArrayValue { env });

    Value *self = new Value { env };
    self->add_main_object_flag();
    self->define_singleton_method(env, "inspect", main_obj_inspect);
    env->global_set("$NAT_main_object", self);

    Value *stdin_fileno = new IntegerValue { env, STDIN_FILENO };
    Value *stdin = new IoValue { env };
    stdin->initialize(env, 1, &stdin_fileno, nullptr);
    env->global_set("$stdin", stdin);
    Object->const_set(env, "STDIN", stdin);

    Value *stdout_fileno = new IntegerValue { env, STDOUT_FILENO };
    Value *stdout = new IoValue { env };
    stdout->initialize(env, 1, &stdout_fileno, nullptr);
    env->global_set("$stdout", stdout);
    Object->const_set(env, "STDOUT", stdout);

    Value *stderr_fileno = new IntegerValue { env, STDERR_FILENO };
    Value *stderr = new IoValue { env };
    stderr->initialize(env, 1, &stderr_fileno, nullptr);
    env->global_set("$stderr", stderr);
    Object->const_set(env, "STDERR", stderr);

    Value *ENV = new Value { env };
    Object->const_set(env, "ENV", ENV);
    NAT_ENV_INIT(ENV);

    Value *RUBY_VERSION = new StringValue { env, "2.7.1" };
    Object->const_set(env, "RUBY_VERSION", RUBY_VERSION);

    init_bindings(env);

    /*OBJ_NAT_INIT*/

    return env;
}

/*TOP*/

extern "C" Value *EVAL(Env *env) {
    Value *self = env->global_get("$NAT_main_object");
    (void)self; // don't warn about unused var
    volatile bool run_exit_handlers = true;
    try {
        /*BODY*/
        run_exit_handlers = false;
        run_at_exit_handlers(env);
        return NAT_NIL; // just in case there's no return value
    } catch (ExceptionValue *exception) {
        handle_top_level_exception(env, exception, run_exit_handlers);
        return NAT_NIL;
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, nullptr, _IOLBF, 1024);
    Env *env = build_top_env();
    ArrayValue *ARGV = new ArrayValue { env };
    /*INIT*/
    NAT_OBJECT->const_set(env, "ARGV", ARGV);
    assert(argc > 0);
    for (int i = 1; i < argc; i++) {
        ARGV->push(new StringValue { env, argv[i] });
    }
    Value *result = EVAL(env);
    delete env->global_env();
    env->clear_global_env();
    delete env;
    if (result) {
        return 0;
    } else {
        return 1;
    }
}
