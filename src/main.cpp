#include "natalie.hpp"

using namespace Natalie;

/* end of front matter */

/*NAT_OBJ*/

extern "C" Env *build_top_env() {
    GlobalEnv *global_env = new GlobalEnv {};
    Env *env = new Env { global_env };
    env->set_method_name(GC_STRDUP("<main>"));

    ClassValue *Class = ClassValue::bootstrap_class_class(env);

    ClassValue *BasicObject = ClassValue::bootstrap_basic_object(env, Class);
    BasicObject->define_singleton_method(env, "new", Value::_new);

    ClassValue *Object = BasicObject->subclass(env, "Object");
    env->global_env()->set_Object(Object);

    // these must be defined after Object exists
    Object->const_set(env, "Class", Class);
    Object->const_set(env, "BasicObject", BasicObject);
    Object->const_set(env, "Object", Object);

    ClassValue *Module = Object->subclass(env, "Module", Value::Type::Module);
    Object->const_set(env, "Module", Module);
    Class->set_superclass_DANGEROUSLY(Module);
    Class->set_singleton_class(Module->singleton_class()->subclass(env, "#<Class:Class>"));

    ModuleValue *Kernel = new ModuleValue { env, "Kernel" };
    Object->const_set(env, "Kernel", Kernel);
    Object->include_once(env, Kernel);

    ModuleValue *Comparable = new ModuleValue { env, "Comparable" };
    Object->const_set(env, "Comparable", Comparable);

    ModuleValue *Enumerable = new ModuleValue { env, "Enumerable" };
    Object->const_set(env, "Enumerable", Enumerable);

    ClassValue *Symbol = Object->subclass(env, "Symbol", Value::Type::Symbol);
    Object->const_set(env, "Symbol", Symbol);

    ClassValue *NilClass = Object->subclass(env, "NilClass", Value::Type::Nil);
    Object->const_set(env, "NilClass", NilClass);

    env->global_env()->set_nil_obj(new NilValue { env });
    env->nil_obj()->set_singleton_class(NilClass);

    ClassValue *TrueClass = Object->subclass(env, "TrueClass", Value::Type::True);
    Object->const_set(env, "TrueClass", TrueClass);

    env->global_env()->set_true_obj(new TrueValue { env });
    env->true_obj()->set_singleton_class(TrueClass);

    ClassValue *FalseClass = Object->subclass(env, "FalseClass", Value::Type::False);
    Object->const_set(env, "FalseClass", FalseClass);

    env->global_env()->set_false_obj(new FalseValue { env });
    env->false_obj()->set_singleton_class(FalseClass);

    ClassValue *Fiber = Object->subclass(env, "Fiber", Value::Type::Fiber);
    Object->const_set(env, "Fiber", Fiber);

    ClassValue *Numeric = Object->subclass(env, "Numeric");
    Object->const_set(env, "Numeric", Numeric);
    Numeric->include_once(env, Comparable);

    ClassValue *Integer = Numeric->subclass(env, "Integer", Value::Type::Integer);
    env->global_env()->set_Integer(Integer);
    Object->const_set(env, "Integer", Integer);
    Object->const_set(env, "Fixnum", Integer);

    ClassValue *Float = Numeric->subclass(env, "Float", Value::Type::Float);
    Object->const_set(env, "Float", Float);
    Float->include_once(env, Comparable);
    FloatValue::build_constants(env, Float);

    Value *Math = new ModuleValue { env, "Math" };
    Object->const_set(env, "Math", Math);
    Math->const_set(env, "PI", new FloatValue { env, M_PI });

    ClassValue *String = Object->subclass(env, "String", Value::Type::String);
    Object->const_set(env, "String", String);
    String->include_once(env, Comparable);

    ClassValue *Array = Object->subclass(env, "Array", Value::Type::Array);
    env->global_env()->set_Array(Array);
    Object->const_set(env, "Array", Array);
    Array->include_once(env, Enumerable);

    ClassValue *Hash = Object->subclass(env, "Hash", Value::Type::Hash);
    Object->const_set(env, "Hash", Hash);
    Hash->include_once(env, Enumerable);

    ClassValue *Regexp = Object->subclass(env, "Regexp", Value::Type::Regexp);
    Object->const_set(env, "Regexp", Regexp);
    Regexp->const_set(env, "IGNORECASE", new IntegerValue { env, 1 });
    Regexp->const_set(env, "EXTENDED", new IntegerValue { env, 2 });
    Regexp->const_set(env, "MULTILINE", new IntegerValue { env, 4 });

    ClassValue *Range = Object->subclass(env, "Range", Value::Type::Range);
    Object->const_set(env, "Range", Range);

    ClassValue *MatchData = Object->subclass(env, "MatchData", Value::Type::MatchData);
    Object->const_set(env, "MatchData", MatchData);

    ClassValue *Proc = Object->subclass(env, "Proc", Value::Type::Proc);
    Object->const_set(env, "Proc", Proc);

    ClassValue *IO = Object->subclass(env, "IO", Value::Type::Io);
    Object->const_set(env, "IO", IO);

    ClassValue *File = IO->subclass(env, "File");
    Object->const_set(env, "File", File);
    FileValue::build_constants(env, File);

    ClassValue *Exception = Object->subclass(env, "Exception", Value::Type::Exception);
    Object->const_set(env, "Exception", Exception);

    ClassValue *Encoding = env->Object()->subclass(env, "Encoding");
    Object->const_set(env, "Encoding", Encoding);

    Value *Process = new ModuleValue { env, "Process" };
    Object->const_set(env, "Process", Process);

    EncodingValue *EncodingAscii8Bit = new EncodingValue { env, Encoding::ASCII_8BIT, { "ASCII-8BIT", "BINARY" } };
    Encoding->const_set(env, "ASCII_8BIT", EncodingAscii8Bit);

    Value *EncodingUTF8 = new EncodingValue { env, Encoding::UTF_8, { "UTF-8" } };
    Encoding->const_set(env, "UTF_8", EncodingUTF8);

    env->global_set("$NAT_at_exit_handlers", new ArrayValue { env });

    Value *self = new Value { env };
    self->add_main_object_flag();
    env->global_set("$NAT_main_object", self);

    Value *_stdin = new IoValue { env, STDIN_FILENO };
    env->global_set("$stdin", _stdin);
    Object->const_set(env, "STDIN", _stdin);

    Value *_stdout = new IoValue { env, STDOUT_FILENO };
    env->global_set("$stdout", _stdout);
    Object->const_set(env, "STDOUT", _stdout);

    Value *_stderr = new IoValue { env, STDERR_FILENO };
    env->global_set("$stderr", _stderr);
    Object->const_set(env, "STDERR", _stderr);

    Value *ENV = new Value { env };
    Object->const_set(env, "ENV", ENV);

    ClassValue *Parser = env->Object()->subclass(env, "Parser");
    Object->const_set(env, "Parser", Parser);

    ClassValue *Sexp = Array->subclass(env, "Sexp", Value::Type::Array);
    Parser->const_set(env, "Sexp", Sexp);

    Value *RUBY_VERSION = new StringValue { env, "2.7.1" };
    Object->const_set(env, "RUBY_VERSION", RUBY_VERSION);

    Value *RUBY_ENGINE = new StringValue { env, "natalie" };
    Object->const_set(env, "RUBY_ENGINE", RUBY_ENGINE);

    init_bindings(env);

    /*NAT_OBJ_INIT*/

    return env;
}

/*NAT_TOP*/

extern "C" Value *EVAL(Env *env) {
    Value *self = env->global_get("$NAT_main_object");
    (void)self; // don't warn about unused var
    volatile bool run_exit_handlers = true;
    try {
        /*NAT_BODY*/
        run_exit_handlers = false;
        run_at_exit_handlers(env);
        return env->nil_obj(); // just in case there's no return value
    } catch (ExceptionValue *exception) {
        handle_top_level_exception(env, exception, run_exit_handlers);
        return env->nil_obj();
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, nullptr, _IOLBF, 1024);
    Env *env = build_top_env();
    ArrayValue *ARGV = new ArrayValue { env };
    /*NAT_INIT*/
    env->Object()->const_set(env, "ARGV", ARGV);
    assert(argc > 0);
    for (int i = 1; i < argc; i++) {
        ARGV->push(new StringValue { env, argv[i] });
    }
    Value *result = EVAL(env);
    env->clear_global_env();
    if (result) {
        return 0;
    } else {
        return 1;
    }
}
