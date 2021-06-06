#include "natalie.hpp"

using namespace Natalie;

/*NAT_DECLARATIONS*/

extern "C" Env *build_top_env() {
    auto *global_env = GlobalEnv::the();
    auto *env = new Env {};
    env->set_is_main(true);

    ClassValue *Class = ClassValue::bootstrap_class_class(env);
    global_env->set_Class(Class);

    ClassValue *BasicObject = ClassValue::bootstrap_basic_object(env, Class);

    ClassValue *Object = BasicObject->subclass(env, "Object");

    global_env->set_Object(Object);

    ClassValue *Symbol = Object->subclass(env, "Symbol", Value::Type::Symbol);
    global_env->set_Symbol(Symbol);
    Object->const_set(env, SymbolValue::intern(env, "Symbol"), Symbol);

    // these must be defined after Object exists
    Object->const_set(env, SymbolValue::intern(env, "Class"), Class);
    Object->const_set(env, SymbolValue::intern(env, "BasicObject"), BasicObject);
    Object->const_set(env, SymbolValue::intern(env, "Object"), Object);

    ClassValue *Module = Object->subclass(env, "Module", Value::Type::Module);
    global_env->set_Module(Module);
    Object->const_set(env, SymbolValue::intern(env, "Module"), Module);
    Class->set_superclass_DANGEROUSLY(Module);
    Class->set_singleton_class(Module->singleton_class()->subclass(env, "#<Class:Class>"));

    ModuleValue *Kernel = new ModuleValue { env, "Kernel" };
    Object->const_set(env, SymbolValue::intern(env, "Kernel"), Kernel);
    Object->include_once(env, Kernel);

    ModuleValue *Comparable = new ModuleValue { env, "Comparable" };
    Object->const_set(env, SymbolValue::intern(env, "Comparable"), Comparable);

    ModuleValue *Enumerable = new ModuleValue { env, "Enumerable" };
    Object->const_set(env, SymbolValue::intern(env, "Enumerable"), Enumerable);

    BasicObject->define_singleton_method(env, SymbolValue::intern(env, "new"), Value::_new, -1);

    ClassValue *NilClass = Object->subclass(env, "NilClass", Value::Type::Nil);
    Object->const_set(env, SymbolValue::intern(env, "NilClass"), NilClass);

    global_env->set_nil_obj(new NilValue { env });
    env->nil_obj()->set_singleton_class(NilClass);

    ClassValue *TrueClass = Object->subclass(env, "TrueClass", Value::Type::True);
    Object->const_set(env, SymbolValue::intern(env, "TrueClass"), TrueClass);

    global_env->set_true_obj(new TrueValue { env });
    env->true_obj()->set_singleton_class(TrueClass);

    ClassValue *FalseClass = Object->subclass(env, "FalseClass", Value::Type::False);
    Object->const_set(env, SymbolValue::intern(env, "FalseClass"), FalseClass);

    global_env->set_false_obj(new FalseValue { env });
    env->false_obj()->set_singleton_class(FalseClass);

    ClassValue *Fiber = Object->subclass(env, "Fiber", Value::Type::Fiber);
    Object->const_set(env, SymbolValue::intern(env, "Fiber"), Fiber);

    ClassValue *Numeric = Object->subclass(env, "Numeric");
    Object->const_set(env, SymbolValue::intern(env, "Numeric"), Numeric);
    Numeric->include_once(env, Comparable);

    ClassValue *Integer = Numeric->subclass(env, "Integer", Value::Type::Integer);
    global_env->set_Integer(Integer);
    Object->const_set(env, SymbolValue::intern(env, "Integer"), Integer);
    Object->const_set(env, SymbolValue::intern(env, "Fixnum"), Integer);

    ClassValue *Float = Numeric->subclass(env, "Float", Value::Type::Float);
    Object->const_set(env, SymbolValue::intern(env, "Float"), Float);
    Float->include_once(env, Comparable);
    FloatValue::build_constants(env, Float);

    ValuePtr Math = new ModuleValue { env, "Math" };
    Object->const_set(env, SymbolValue::intern(env, "Math"), Math);
    Math->const_set(env, SymbolValue::intern(env, "PI"), new FloatValue { env, M_PI });

    ClassValue *String = Object->subclass(env, "String", Value::Type::String);
    global_env->set_String(String);
    Object->const_set(env, SymbolValue::intern(env, "String"), String);
    String->include_once(env, Comparable);

    ClassValue *Array = Object->subclass(env, "Array", Value::Type::Array);
    global_env->set_Array(Array);
    Object->const_set(env, SymbolValue::intern(env, "Array"), Array);
    Array->include_once(env, Enumerable);

    ClassValue *Hash = Object->subclass(env, "Hash", Value::Type::Hash);
    global_env->set_Hash(Hash);
    Object->const_set(env, SymbolValue::intern(env, "Hash"), Hash);
    Hash->include_once(env, Enumerable);

    ClassValue *Regexp = Object->subclass(env, "Regexp", Value::Type::Regexp);
    global_env->set_Regexp(Regexp);
    Object->const_set(env, SymbolValue::intern(env, "Regexp"), Regexp);
    Regexp->const_set(env, SymbolValue::intern(env, "IGNORECASE"), ValuePtr { env, 1 });
    Regexp->const_set(env, SymbolValue::intern(env, "EXTENDED"), ValuePtr { env, 2 });
    Regexp->const_set(env, SymbolValue::intern(env, "MULTILINE"), ValuePtr { env, 4 });

    ClassValue *Range = Object->subclass(env, "Range", Value::Type::Range);
    Object->const_set(env, SymbolValue::intern(env, "Range"), Range);
    Range->include_once(env, Enumerable);

    ClassValue *MatchData = Object->subclass(env, "MatchData", Value::Type::MatchData);
    Object->const_set(env, SymbolValue::intern(env, "MatchData"), MatchData);

    ClassValue *Proc = Object->subclass(env, "Proc", Value::Type::Proc);
    Object->const_set(env, SymbolValue::intern(env, "Proc"), Proc);

    ClassValue *IO = Object->subclass(env, "IO", Value::Type::Io);
    Object->const_set(env, SymbolValue::intern(env, "IO"), IO);

    ClassValue *File = IO->subclass(env, "File");
    Object->const_set(env, SymbolValue::intern(env, "File"), File);
    FileValue::build_constants(env, File);

    ClassValue *Exception = Object->subclass(env, "Exception", Value::Type::Exception);
    Object->const_set(env, SymbolValue::intern(env, "Exception"), Exception);

    ClassValue *Encoding = env->Object()->subclass(env, "Encoding");
    Object->const_set(env, SymbolValue::intern(env, "Encoding"), Encoding);

    EncodingValue *EncodingAscii8Bit = new EncodingValue { env, Encoding::ASCII_8BIT, { "ASCII-8BIT", "BINARY" } };
    Encoding->const_set(env, SymbolValue::intern(env, "ASCII_8BIT"), EncodingAscii8Bit);

    ValuePtr EncodingUTF8 = new EncodingValue { env, Encoding::UTF_8, { "UTF-8" } };
    Encoding->const_set(env, SymbolValue::intern(env, "UTF_8"), EncodingUTF8);

    ValuePtr Process = new ModuleValue { env, "Process" };
    Object->const_set(env, SymbolValue::intern(env, "Process"), Process);

    ClassValue *Method = Object->subclass(env, "Method", Value::Type::Method);
    Object->const_set(env, SymbolValue::intern(env, "Method"), Method);

    env->global_set(SymbolValue::intern(env, "$NAT_at_exit_handlers"), new ArrayValue { env });

    ValuePtr self = new Value { env };
    self->add_main_object_flag();
    env->global_set(SymbolValue::intern(env, "$NAT_main_object"), self);

    ValuePtr _stdin = new IoValue { env, STDIN_FILENO };
    env->global_set(SymbolValue::intern(env, "$stdin"), _stdin);
    Object->const_set(env, SymbolValue::intern(env, "STDIN"), _stdin);

    ValuePtr _stdout = new IoValue { env, STDOUT_FILENO };
    env->global_set(SymbolValue::intern(env, "$stdout"), _stdout);
    Object->const_set(env, SymbolValue::intern(env, "STDOUT"), _stdout);

    ValuePtr _stderr = new IoValue { env, STDERR_FILENO };
    env->global_set(SymbolValue::intern(env, "$stderr"), _stderr);
    Object->const_set(env, SymbolValue::intern(env, "STDERR"), _stderr);

    ValuePtr ENV = new Value { env };
    Object->const_set(env, SymbolValue::intern(env, "ENV"), ENV);

    ClassValue *Parser = env->Object()->subclass(env, "Parser");
    Object->const_set(env, SymbolValue::intern(env, "Parser"), Parser);

    ClassValue *Sexp = Array->subclass(env, "Sexp", Value::Type::Array);
    Parser->const_set(env, SymbolValue::intern(env, "Sexp"), Sexp);

    ValuePtr RUBY_VERSION = new StringValue { env, "2.7.1" };
    Object->const_set(env, SymbolValue::intern(env, "RUBY_VERSION"), RUBY_VERSION);

    ValuePtr RUBY_ENGINE = new StringValue { env, "natalie" };
    Object->const_set(env, SymbolValue::intern(env, "RUBY_ENGINE"), RUBY_ENGINE);

    StringValue *RUBY_PLATFORM = new StringValue { env, ruby_platform };
    Object->const_set(env, SymbolValue::intern(env, "RUBY_PLATFORM"), RUBY_PLATFORM);

    init_bindings(env);

    /*NAT_OBJ_INIT*/

    return env;
}

extern "C" Value *EVAL(Env *env) {
    /*NAT_EVAL_INIT*/

    ValuePtr self = env->global_get(SymbolValue::intern(env, "$NAT_main_object"));
    (void)self; // don't warn about unused var
    volatile bool run_exit_handlers = true;
    try {
        /*NAT_EVAL_BODY*/
        run_exit_handlers = false;
        run_at_exit_handlers(env);
        return env->nil_obj(); // just in case there's no return value
    } catch (ExceptionValue *exception) {
        handle_top_level_exception(env, exception, run_exit_handlers);
        return env->nil_obj();
    }
}

ValuePtr _main(int argc, char *argv[]) {
    Env *env = build_top_env();
    GlobalEnv::the()->main_fiber(env);

#ifndef NAT_GC_DISABLE
    Heap::the().gc_enable();
#endif

    assert(argc > 0);
    ValuePtr exe = new StringValue { env, argv[0] };
    env->global_set(SymbolValue::intern(env, "$exe"), exe);

    ArrayValue *ARGV = new ArrayValue { env };
    env->Object()->const_set(env, SymbolValue::intern(env, "ARGV"), ARGV);
    for (int i = 1; i < argc; i++) {
        ARGV->push(new StringValue { env, argv[i] });
    }

    return EVAL(env);
}

int main(int argc, char *argv[]) {
    Heap::the().set_start_of_stack(&argv);
    setvbuf(stdout, nullptr, _IOLBF, 1024);
    if (_main(argc, argv))
        return 0;
    return 1;
}
