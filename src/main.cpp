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
    Object->const_set(SymbolValue::intern("Symbol"), Symbol);

    // these must be defined after Object exists
    Object->const_set(SymbolValue::intern("Class"), Class);
    Object->const_set(SymbolValue::intern("BasicObject"), BasicObject);
    Object->const_set(SymbolValue::intern("Object"), Object);

    ClassValue *Module = Object->subclass(env, "Module", Value::Type::Module);
    global_env->set_Module(Module);
    Object->const_set(SymbolValue::intern("Module"), Module);
    Class->set_superclass_DANGEROUSLY(Module);
    Class->set_singleton_class(Module->singleton_class()->subclass(env, "#<Class:Class>"));

    ModuleValue *Kernel = new ModuleValue { "Kernel" };
    Object->const_set(SymbolValue::intern("Kernel"), Kernel);
    Object->include_once(env, Kernel);

    ModuleValue *Comparable = new ModuleValue { "Comparable" };
    Object->const_set(SymbolValue::intern("Comparable"), Comparable);

    ModuleValue *Enumerable = new ModuleValue { "Enumerable" };
    Object->const_set(SymbolValue::intern("Enumerable"), Enumerable);

    BasicObject->define_singleton_method(env, SymbolValue::intern("new"), Value::_new, -1);

    ClassValue *NilClass = Object->subclass(env, "NilClass", Value::Type::Nil);
    Object->const_set(SymbolValue::intern("NilClass"), NilClass);
    NilValue::the();

    ClassValue *TrueClass = Object->subclass(env, "TrueClass", Value::Type::True);
    Object->const_set(SymbolValue::intern("TrueClass"), TrueClass);
    TrueValue::the();

    ClassValue *FalseClass = Object->subclass(env, "FalseClass", Value::Type::False);
    Object->const_set(SymbolValue::intern("FalseClass"), FalseClass);
    FalseValue::the();

    ClassValue *Fiber = Object->subclass(env, "Fiber", Value::Type::Fiber);
    Object->const_set(SymbolValue::intern("Fiber"), Fiber);

    ClassValue *Numeric = Object->subclass(env, "Numeric");
    Object->const_set(SymbolValue::intern("Numeric"), Numeric);
    Numeric->include_once(env, Comparable);

    ClassValue *Integer = Numeric->subclass(env, "Integer", Value::Type::Integer);
    global_env->set_Integer(Integer);
    Object->const_set(SymbolValue::intern("Integer"), Integer);
    Object->const_set(SymbolValue::intern("Fixnum"), Integer);

    ClassValue *Float = Numeric->subclass(env, "Float", Value::Type::Float);
    global_env->set_Float(Float);
    Object->const_set(SymbolValue::intern("Float"), Float);
    Float->include_once(env, Comparable);
    FloatValue::build_constants(env, Float);

    ValuePtr Math = new ModuleValue { "Math" };
    Object->const_set(SymbolValue::intern("Math"), Math);
    Math->const_set(SymbolValue::intern("PI"), new FloatValue { M_PI });

    ClassValue *String = Object->subclass(env, "String", Value::Type::String);
    global_env->set_String(String);
    Object->const_set(SymbolValue::intern("String"), String);
    String->include_once(env, Comparable);

    ClassValue *Array = Object->subclass(env, "Array", Value::Type::Array);
    global_env->set_Array(Array);
    Object->const_set(SymbolValue::intern("Array"), Array);
    Array->include_once(env, Enumerable);

    ClassValue *Hash = Object->subclass(env, "Hash", Value::Type::Hash);
    global_env->set_Hash(Hash);
    Object->const_set(SymbolValue::intern("Hash"), Hash);
    Hash->include_once(env, Enumerable);

    ClassValue *Regexp = Object->subclass(env, "Regexp", Value::Type::Regexp);
    global_env->set_Regexp(Regexp);
    Object->const_set(SymbolValue::intern("Regexp"), Regexp);
    Regexp->const_set(SymbolValue::intern("IGNORECASE"), ValuePtr::integer(1));
    Regexp->const_set(SymbolValue::intern("EXTENDED"), ValuePtr::integer(2));
    Regexp->const_set(SymbolValue::intern("MULTILINE"), ValuePtr::integer(4));

    ClassValue *Range = Object->subclass(env, "Range", Value::Type::Range);
    Object->const_set(SymbolValue::intern("Range"), Range);
    Range->include_once(env, Enumerable);

    ClassValue *MatchData = Object->subclass(env, "MatchData", Value::Type::MatchData);
    Object->const_set(SymbolValue::intern("MatchData"), MatchData);

    ClassValue *Proc = Object->subclass(env, "Proc", Value::Type::Proc);
    Object->const_set(SymbolValue::intern("Proc"), Proc);

    ClassValue *IO = Object->subclass(env, "IO", Value::Type::Io);
    Object->const_set(SymbolValue::intern("IO"), IO);

    ClassValue *File = IO->subclass(env, "File");
    Object->const_set(SymbolValue::intern("File"), File);
    FileValue::build_constants(env, File);

    ClassValue *Exception = Object->subclass(env, "Exception", Value::Type::Exception);
    Object->const_set(SymbolValue::intern("Exception"), Exception);

    ClassValue *Encoding = GlobalEnv::the()->Object()->subclass(env, "Encoding");
    Object->const_set(SymbolValue::intern("Encoding"), Encoding);

    EncodingValue *EncodingAscii8Bit = new EncodingValue { Encoding::ASCII_8BIT, { "ASCII-8BIT", "BINARY" } };
    Encoding->const_set(SymbolValue::intern("ASCII_8BIT"), EncodingAscii8Bit);

    ValuePtr EncodingUTF8 = new EncodingValue { Encoding::UTF_8, { "UTF-8" } };
    Encoding->const_set(SymbolValue::intern("UTF_8"), EncodingUTF8);

    ValuePtr Process = new ModuleValue { "Process" };
    Object->const_set(SymbolValue::intern("Process"), Process);

    ClassValue *Method = Object->subclass(env, "Method", Value::Type::Method);
    Object->const_set(SymbolValue::intern("Method"), Method);

    env->global_set(SymbolValue::intern("$NAT_at_exit_handlers"), new ArrayValue {});

    ValuePtr self = new Value {};
    self->add_main_object_flag();
    env->global_set(SymbolValue::intern("$NAT_main_object"), self);

    ValuePtr _stdin = new IoValue { STDIN_FILENO };
    env->global_set(SymbolValue::intern("$stdin"), _stdin);
    Object->const_set(SymbolValue::intern("STDIN"), _stdin);

    ValuePtr _stdout = new IoValue { STDOUT_FILENO };
    env->global_set(SymbolValue::intern("$stdout"), _stdout);
    Object->const_set(SymbolValue::intern("STDOUT"), _stdout);

    ValuePtr _stderr = new IoValue { STDERR_FILENO };
    env->global_set(SymbolValue::intern("$stderr"), _stderr);
    Object->const_set(SymbolValue::intern("STDERR"), _stderr);

    ValuePtr ENV = new Value {};
    Object->const_set(SymbolValue::intern("ENV"), ENV);

    ClassValue *Parser = GlobalEnv::the()->Object()->subclass(env, "Parser");
    Object->const_set(SymbolValue::intern("Parser"), Parser);

    ClassValue *Sexp = Array->subclass(env, "Sexp", Value::Type::Array);
    Parser->const_set(SymbolValue::intern("Sexp"), Sexp);

    ValuePtr RUBY_VERSION = new StringValue { "2.7.1" };
    Object->const_set(SymbolValue::intern("RUBY_VERSION"), RUBY_VERSION);

    ValuePtr RUBY_ENGINE = new StringValue { "natalie" };
    Object->const_set(SymbolValue::intern("RUBY_ENGINE"), RUBY_ENGINE);

    StringValue *RUBY_PLATFORM = new StringValue { ruby_platform };
    Object->const_set(SymbolValue::intern("RUBY_PLATFORM"), RUBY_PLATFORM);

    init_bindings(env);

    /*NAT_OBJ_INIT*/

    return env;
}

extern "C" Value *EVAL(Env *env) {
    /*NAT_EVAL_INIT*/

    ValuePtr self = env->global_get(SymbolValue::intern("$NAT_main_object"));
    (void)self; // don't warn about unused var
    volatile bool run_exit_handlers = true;
    try {
        /*NAT_EVAL_BODY*/
        run_exit_handlers = false;
        run_at_exit_handlers(env);
        return NilValue::the(); // just in case there's no return value
    } catch (ExceptionValue *exception) {
        handle_top_level_exception(env, exception, run_exit_handlers);
        return NilValue::the();
    }
}

ValuePtr _main(int argc, char *argv[]) {
    Env *env = build_top_env();
    GlobalEnv::the()->main_fiber(env);

#ifndef NAT_GC_DISABLE
    Heap::the().gc_enable();
#endif

    assert(argc > 0);
    ValuePtr exe = new StringValue { argv[0] };
    env->global_set(SymbolValue::intern("$exe"), exe);

    ArrayValue *ARGV = new ArrayValue {};
    GlobalEnv::the()->Object()->const_set(SymbolValue::intern("ARGV"), ARGV);
    for (int i = 1; i < argc; i++) {
        ARGV->push(new StringValue { argv[i] });
    }

    return EVAL(env);
}

int main(int argc, char *argv[]) {
    Heap::the().set_start_of_stack(&argv);
    setvbuf(stdout, nullptr, _IOLBF, 1024);
    auto return_code = _main(argc, argv) ? 0 : 1;
#ifdef NAT_GC_COLLECT_ALL_AT_EXIT
    Heap::the().collect_all();
#endif
    return return_code;
}
