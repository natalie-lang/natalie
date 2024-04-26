#include "natalie.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_variable_info/access_hooks.hpp"
#include "natalie/value.hpp"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <sys/wait.h>

namespace Natalie {

Env *build_top_env() {
    auto *global_env = GlobalEnv::the();
    auto *env = new Env {};
    global_env->set_main_env(env);

    ClassObject *Class = ClassObject::bootstrap_class_class(env);
    global_env->set_Class(Class);

    ClassObject *BasicObject = ClassObject::bootstrap_basic_object(env, Class);
    global_env->set_BasicObject(BasicObject);

    ClassObject *Object = BasicObject->subclass(env, "Object");

    global_env->set_Object(Object);

    ClassObject *Symbol = Object->subclass(env, "Symbol", Object::Type::Symbol);
    global_env->set_Symbol(Symbol);
    Object->const_set("Symbol"_s, Symbol);

    // these must be defined after Object exists
    Object->const_set("Class"_s, Class);
    Object->const_set("BasicObject"_s, BasicObject);
    Object->const_set("Object"_s, Object);

    ClassObject *Module = Object->subclass(env, "Module", Object::Type::Module);
    global_env->set_Module(Module);
    Object->const_set("Module"_s, Module);
    Class->set_superclass_DANGEROUSLY(Module);
    auto class_singleton_class = new ClassObject { Module->singleton_class() };
    Module->singleton_class()->initialize_subclass_without_checks(class_singleton_class, env, "#<Class:Class>");
    Class->set_singleton_class(class_singleton_class);

    ModuleObject *Kernel = new ModuleObject { "Kernel" };
    Object->const_set("Kernel"_s, Kernel);
    Object->include_once(env, Kernel);

    ModuleObject *Comparable = new ModuleObject { "Comparable" };
    Object->const_set("Comparable"_s, Comparable);
    Symbol->include_once(env, Comparable);

    ModuleObject *Enumerable = new ModuleObject { "Enumerable" };
    Object->const_set("Enumerable"_s, Enumerable);

    ClassObject *Enumerator = Object->subclass(env, "Enumerator", Object::Type::Enumerator);
    Object->const_set("Enumerator"_s, Enumerator);

    ClassObject *EnumeratorArithmeticSequence = Enumerator->subclass(env, "ArithmeticSequence", Object::Type::EnumeratorArithmeticSequence);
    Enumerator->const_set("ArithmeticSequence"_s, EnumeratorArithmeticSequence);

    BasicObject->define_singleton_method(env, "new"_s, Object::_new, -1);
    BasicObject->define_singleton_method(env, "allocate"_s, Object::allocate, -1);

    ClassObject *NilClass = Object->subclass(env, "NilClass", Object::Type::Nil);
    Object->const_set("NilClass"_s, NilClass);
    NilObject::the()->set_singleton_class(NilClass);
    NilClass->set_is_singleton(false);

    ClassObject *TrueClass = Object->subclass(env, "TrueClass", Object::Type::True);
    Object->const_set("TrueClass"_s, TrueClass);
    TrueObject::the()->set_singleton_class(TrueClass);
    TrueClass->set_is_singleton(false);

    ClassObject *FalseClass = Object->subclass(env, "FalseClass", Object::Type::False);
    Object->const_set("FalseClass"_s, FalseClass);
    FalseObject::the()->set_singleton_class(FalseClass);
    FalseClass->set_is_singleton(false);

    ClassObject *Fiber = Object->subclass(env, "Fiber", Object::Type::Fiber);
    Object->const_set("Fiber"_s, Fiber);

    ClassObject *Numeric = Object->subclass(env, "Numeric");
    Object->const_set("Numeric"_s, Numeric);
    Numeric->include_once(env, Comparable);

    ClassObject *Integer = Numeric->subclass(env, "Integer", Object::Type::Integer);
    global_env->set_Integer(Integer);
    Object->const_set("Integer"_s, Integer);

    ClassObject *Float = Numeric->subclass(env, "Float", Object::Type::Float);
    global_env->set_Float(Float);
    Object->const_set("Float"_s, Float);
    Float->include_once(env, Comparable);
    FloatObject::build_constants(env, Float);

    ClassObject *Rational = Numeric->subclass(env, "Rational", Object::Type::Rational);
    global_env->set_Rational(Rational);
    Object->const_set("Rational"_s, Rational);

    Value Math = new ModuleObject { "Math" };
    Object->const_set("Math"_s, Math);
    Math->const_set("E"_s, new FloatObject { M_E });
    Math->const_set("PI"_s, new FloatObject { M_PI });

    ModuleObject *Signal = new ModuleObject { "Signal" };
    Object->const_set("Signal"_s, Signal);

    ClassObject *String = Object->subclass(env, "String", Object::Type::String);
    global_env->set_String(String);
    Object->const_set("String"_s, String);
    String->include_once(env, Comparable);

    ClassObject *Array = Object->subclass(env, "Array", Object::Type::Array);
    global_env->set_Array(Array);
    Object->const_set("Array"_s, Array);
    Array->include_once(env, Enumerable);

    ClassObject *Binding = Object->subclass(env, "Binding", Object::Type::Binding);
    global_env->set_Binding(Binding);
    Object->const_set("Binding"_s, Binding);

    ClassObject *Hash = Object->subclass(env, "Hash", Object::Type::Hash);
    global_env->set_Hash(Hash);
    Object->const_set("Hash"_s, Hash);
    Hash->include_once(env, Enumerable);

    ClassObject *Random = Object->subclass(env, "Random", Object::Type::Random);
    global_env->set_Random(Random);
    Object->const_set("Random"_s, Random);
    Random->const_set("DEFAULT"_s, (new RandomObject)->initialize(env, nullptr));

    ClassObject *Regexp = Object->subclass(env, "Regexp", Object::Type::Regexp);
    global_env->set_Regexp(Regexp);
    Object->const_set("Regexp"_s, Regexp);
#define REGEXP_CONSTS(cpp_name, ruby_name, bits) Regexp->const_set(SymbolObject::intern(#ruby_name), Value::integer(bits));
    ENUMERATE_REGEX_OPTS(REGEXP_CONSTS)
#undef REGEXP_CONSTS

    ClassObject *Range = Object->subclass(env, "Range", Object::Type::Range);
    Object->const_set("Range"_s, Range);
    Range->include_once(env, Enumerable);

    ClassObject *Complex = Numeric->subclass(env, "Complex", Object::Type::Complex);
    global_env->set_Complex(Complex);
    Object->const_set("Complex"_s, Complex);

    ClassObject *Time = Object->subclass(env, "Time", Object::Type::Time);
    global_env->set_Time(Time);
    Object->const_set("Time"_s, Time);

    ClassObject *MatchData = Object->subclass(env, "MatchData", Object::Type::MatchData);
    Object->const_set("MatchData"_s, MatchData);

    ClassObject *Proc = Object->subclass(env, "Proc", Object::Type::Proc);
    Object->const_set("Proc"_s, Proc);

    ClassObject *IO = Object->subclass(env, "IO", Object::Type::Io);
    Object->const_set("IO"_s, IO);
    IO->include_once(env, Enumerable);
    IoObject::build_constants(env, IO);

    ClassObject *File = IO->subclass(env, "File");
    Object->const_set("File"_s, File);
    File->include_once(env, Enumerable);

    ModuleObject *FileTest = new ModuleObject { "FileTest" };
    Object->const_set("FileTest"_s, FileTest);

    ClassObject *FileStat = Object->subclass(env, "Stat", Object::Type::FileStat);
    File->const_set("Stat"_s, FileStat);
    FileStat->include_once(env, Comparable);

    ClassObject *Dir = Object->subclass(env, "Dir", Object::Type::Dir);
    Object->const_set("Dir"_s, Dir);
    Dir->include_once(env, Enumerable);

    ClassObject *Exception = Object->subclass(env, "Exception", Object::Type::Exception);
    Object->const_set("Exception"_s, Exception);
    ClassObject *ScriptError = Exception->subclass(env, "ScriptError", Object::Type::Exception);
    Object->const_set("ScriptError"_s, ScriptError);
    ClassObject *SyntaxError = ScriptError->subclass(env, "SyntaxError", Object::Type::Exception);
    Object->const_set("SyntaxError"_s, SyntaxError);
    ClassObject *StandardError = Exception->subclass(env, "StandardError", Object::Type::Exception);
    Object->const_set("StandardError"_s, StandardError);
    ClassObject *NameError = StandardError->subclass(env, "NameError", Object::Type::Exception);
    Object->const_set("NameError"_s, NameError);

    ClassObject *Encoding = GlobalEnv::the()->Object()->subclass(env, "Encoding");
    Object->const_set("Encoding"_s, Encoding);

    Value EncodingAscii8Bit = new Ascii8BitEncodingObject {};
    Encoding->const_set("ASCII_8BIT"_s, EncodingAscii8Bit);
    Encoding->const_set("BINARY"_s, EncodingAscii8Bit);

    Value EncodingUsAscii = new UsAsciiEncodingObject {};
    Encoding->const_set("US_ASCII"_s, EncodingUsAscii);
    Encoding->const_set("ASCII"_s, EncodingUsAscii);
    Encoding->const_set("ANSI_X3_4_1968"_s, EncodingUsAscii);

    Value EncodingUTF8 = new Utf8EncodingObject {};
    Encoding->const_set("UTF_8"_s, EncodingUTF8);
    Encoding->const_set("CP65001"_s, EncodingUTF8);

    Value EncodingUTF16LE = new Utf16LeEncodingObject {};
    Encoding->const_set("UTF_16LE"_s, EncodingUTF16LE);

    Value EncodingUTF16BE = new Utf16BeEncodingObject {};
    Encoding->const_set("UTF_16BE"_s, EncodingUTF16BE);

    Value EncodingUTF32LE = new Utf32LeEncodingObject {};
    Encoding->const_set("UTF_32LE"_s, EncodingUTF32LE);
    Encoding->const_set("UCS_4LE"_s, EncodingUTF32LE);

    Value EncodingUTF32BE = new Utf32BeEncodingObject {};
    Encoding->const_set("UTF_32BE"_s, EncodingUTF32BE);
    Encoding->const_set("UCS_4BE"_s, EncodingUTF32BE);

    Value EncodingIBM037 = new Ibm037EncodingObject {};
    Encoding->const_set("IBM037"_s, EncodingIBM037);
    Value EncodingIBM437 = new Ibm437EncodingObject {};
    Encoding->const_set("IBM437"_s, EncodingIBM437);
    Encoding->const_set("CP437"_s, EncodingIBM437);
    Value EncodingIBM720 = new Ibm720EncodingObject {};
    Encoding->const_set("IBM720"_s, EncodingIBM720);
    Encoding->const_set("CP720"_s, EncodingIBM720);
    Value EncodingIBM737 = new Ibm737EncodingObject {};
    Encoding->const_set("IBM737"_s, EncodingIBM737);
    Encoding->const_set("CP737"_s, EncodingIBM737);
    Value EncodingIBM775 = new Ibm775EncodingObject {};
    Encoding->const_set("IBM775"_s, EncodingIBM775);
    Encoding->const_set("CP775"_s, EncodingIBM775);
    Value EncodingIBM850 = new Ibm850EncodingObject {};
    Encoding->const_set("IBM850"_s, EncodingIBM850);
    Encoding->const_set("CP850"_s, EncodingIBM850);
    Value EncodingIBM852 = new Ibm852EncodingObject {};
    Encoding->const_set("IBM852"_s, EncodingIBM852);
    Encoding->const_set("CP852"_s, EncodingIBM852);
    Value EncodingIBM855 = new Ibm855EncodingObject {};
    Encoding->const_set("IBM855"_s, EncodingIBM855);
    Encoding->const_set("CP855"_s, EncodingIBM855);
    Value EncodingIBM857 = new Ibm857EncodingObject {};
    Encoding->const_set("IBM857"_s, EncodingIBM857);
    Encoding->const_set("CP857"_s, EncodingIBM857);
    Value EncodingIBM860 = new Ibm860EncodingObject {};
    Encoding->const_set("IBM860"_s, EncodingIBM860);
    Encoding->const_set("CP860"_s, EncodingIBM860);
    Value EncodingIBM861 = new Ibm861EncodingObject {};
    Encoding->const_set("IBM861"_s, EncodingIBM861);
    Encoding->const_set("CP861"_s, EncodingIBM861);
    Value EncodingIBM862 = new Ibm862EncodingObject {};
    Encoding->const_set("IBM862"_s, EncodingIBM862);
    Encoding->const_set("CP862"_s, EncodingIBM862);
    Value EncodingIBM863 = new Ibm863EncodingObject {};
    Encoding->const_set("IBM863"_s, EncodingIBM863);
    Encoding->const_set("CP863"_s, EncodingIBM863);
    Value EncodingIBM864 = new Ibm864EncodingObject {};
    Encoding->const_set("IBM864"_s, EncodingIBM864);
    Encoding->const_set("CP864"_s, EncodingIBM864);
    Value EncodingIBM865 = new Ibm865EncodingObject {};
    Encoding->const_set("IBM865"_s, EncodingIBM865);
    Encoding->const_set("CP865"_s, EncodingIBM865);
    Value EncodingIBM866 = new Ibm866EncodingObject {};
    Encoding->const_set("IBM866"_s, EncodingIBM866);
    Encoding->const_set("CP866"_s, EncodingIBM866);
    Value EncodingIBM869 = new Ibm869EncodingObject {};
    Encoding->const_set("IBM869"_s, EncodingIBM869);
    Encoding->const_set("CP869"_s, EncodingIBM869);

    Value EncodingShiftJis = new ShiftJisEncodingObject {};
    Encoding->const_set("Shift_JIS"_s, EncodingShiftJis);
    Encoding->const_set("SHIFT_JIS"_s, EncodingShiftJis);

    Value EncodingEucJp = new EucJpEncodingObject {};
    Encoding->const_set("EUC_JP"_s, EncodingEucJp);
    Encoding->const_set("EUCJP"_s, EncodingEucJp);

    Value EncodingIso88591 = new Iso88591EncodingObject {};
    Encoding->const_set("ISO_8859_1"_s, EncodingIso88591);
    Encoding->const_set("ISO8859_1"_s, EncodingIso88591);

    Value EncodingIso88592 = new Iso88592EncodingObject {};
    Encoding->const_set("ISO_8859_2"_s, EncodingIso88592);
    Encoding->const_set("ISO8859_2"_s, EncodingIso88592);

    Value EncodingIso88593 = new Iso88593EncodingObject {};
    Encoding->const_set("ISO_8859_3"_s, EncodingIso88593);
    Encoding->const_set("ISO8859_3"_s, EncodingIso88593);

    Value EncodingIso88594 = new Iso88594EncodingObject {};
    Encoding->const_set("ISO_8859_4"_s, EncodingIso88594);
    Encoding->const_set("ISO8859_4"_s, EncodingIso88594);

    Value EncodingIso88595 = new Iso88595EncodingObject {};
    Encoding->const_set("ISO_8859_5"_s, EncodingIso88595);
    Encoding->const_set("ISO8859_5"_s, EncodingIso88595);

    Value EncodingIso88596 = new Iso88596EncodingObject {};
    Encoding->const_set("ISO_8859_6"_s, EncodingIso88596);
    Encoding->const_set("ISO8859_6"_s, EncodingIso88596);

    Value EncodingIso88597 = new Iso88597EncodingObject {};
    Encoding->const_set("ISO_8859_7"_s, EncodingIso88597);
    Encoding->const_set("ISO8859_7"_s, EncodingIso88597);

    Value EncodingIso88598 = new Iso88598EncodingObject {};
    Encoding->const_set("ISO_8859_8"_s, EncodingIso88598);
    Encoding->const_set("ISO8859_8"_s, EncodingIso88598);

    Value EncodingIso88599 = new Iso88599EncodingObject {};
    Encoding->const_set("ISO_8859_9"_s, EncodingIso88599);
    Encoding->const_set("ISO8859_9"_s, EncodingIso88599);

    Value EncodingIso885910 = new Iso885910EncodingObject {};
    Encoding->const_set("ISO_8859_10"_s, EncodingIso885910);
    Encoding->const_set("ISO8859_10"_s, EncodingIso885910);

    Value EncodingIso885911 = new Iso885911EncodingObject {};
    Encoding->const_set("ISO_8859_11"_s, EncodingIso885911);
    Encoding->const_set("ISO8859_11"_s, EncodingIso885911);

    Value EncodingIso885913 = new Iso885913EncodingObject {};
    Encoding->const_set("ISO_8859_13"_s, EncodingIso885913);
    Encoding->const_set("ISO8859_13"_s, EncodingIso885913);

    Value EncodingIso885914 = new Iso885914EncodingObject {};
    Encoding->const_set("ISO_8859_14"_s, EncodingIso885914);
    Encoding->const_set("ISO8859_14"_s, EncodingIso885914);

    Value EncodingIso885915 = new Iso885915EncodingObject {};
    Encoding->const_set("ISO_8859_15"_s, EncodingIso885915);
    Encoding->const_set("ISO8859_15"_s, EncodingIso885915);

    Value EncodingIso885916 = new Iso885916EncodingObject {};
    Encoding->const_set("ISO_8859_16"_s, EncodingIso885916);
    Encoding->const_set("ISO8859_16"_s, EncodingIso885916);

    Value EncodingWindows1250 = new Windows1250EncodingObject {};
    Encoding->const_set("Windows_1250"_s, EncodingWindows1250);
    Encoding->const_set("CP1250"_s, EncodingWindows1250);
    Value EncodingWindows1251 = new Windows1251EncodingObject {};
    Encoding->const_set("Windows_1251"_s, EncodingWindows1251);
    Encoding->const_set("CP1251"_s, EncodingWindows1251);
    Value EncodingWindows1252 = new Windows1252EncodingObject {};
    Encoding->const_set("Windows_1252"_s, EncodingWindows1252);
    Encoding->const_set("CP1252"_s, EncodingWindows1252);

    // Must set defaults after the encodings are defined above.
    EncodingObject::initialize_defaults(env);

    ModuleObject *Process = new ModuleObject { "Process" };
    Object->const_set("Process"_s, Process);
    Value ProcessSys = new ModuleObject { "Sys" };
    Process->const_set("Sys"_s, ProcessSys);
    Value ProcessUID = new ModuleObject { "UID" };
    Process->const_set("UID"_s, ProcessUID);
    Value ProcessGID = new ModuleObject { "GID" };
    Process->const_set("GID"_s, ProcessGID);

    ClassObject *Thread = Object->subclass(env, "Thread", Object::Type::Thread);
    Object->const_set("Thread"_s, Thread);

    ModuleObject *ThreadBacktrace = new ModuleObject { "Backtrace" };
    Thread->const_set("Backtrace"_s, ThreadBacktrace);

    ClassObject *ThreadBacktraceLocation = Object->subclass(env, "Location", Object::Type::ThreadBacktraceLocation);
    ThreadBacktrace->const_set("Location"_s, ThreadBacktraceLocation);

    ClassObject *ThreadGroup = Object->subclass(env, "ThreadGroup", Object::Type::ThreadGroup);
    Object->const_set("ThreadGroup"_s, ThreadGroup);
    ThreadGroupObject::initialize_default();
    ThreadGroup->const_set("Default"_s, ThreadGroupObject::get_default());

    ClassObject *ThreadMutex = Object->subclass(env, "Mutex", Object::Type::ThreadMutex);
    Thread->const_set("Mutex"_s, ThreadMutex);
    Object->const_set("Mutex"_s, ThreadMutex);

    ClassObject *Method = Object->subclass(env, "Method", Object::Type::Method);
    Object->const_set("Method"_s, Method);

    ClassObject *UnboundMethod = Object->subclass(env, "UnboundMethod", Object::Type::UnboundMethod);
    Object->const_set("UnboundMethod"_s, UnboundMethod);

    ModuleObject *FileConstants = new ModuleObject { "Constants" };
    File->const_set("Constants"_s, FileConstants);

    // Build File Constants after Encodings are defined since some
    // strings depend on the encoding.
    FileObject::build_constants(env, FileConstants);

    // include File::Constants module in File and IO
    File->include_once(env, FileConstants);
    IO->include_once(env, FileConstants);

    env->global_set("$NAT_at_exit_handlers"_s, new ArrayObject {});

    auto main_obj = new Natalie::Object {};
    GlobalEnv::the()->set_main_obj(main_obj);

    Value _stdin = new IoObject { STDIN_FILENO };
    env->global_set("$stdin"_s, _stdin);
    Object->const_set("STDIN"_s, _stdin);

    Value _stdout = new IoObject { STDOUT_FILENO };
    env->global_set("$stdout"_s, _stdout);
    GlobalEnv::the()->global_set_write_hook(env, "$stdout"_s, GlobalVariableAccessHooks::WriteHooks::set_stdout);
    env->global_alias("$>"_s, "$stdout"_s);
    Object->const_set("STDOUT"_s, _stdout);

    Value _stderr = new IoObject { STDERR_FILENO };
    env->global_set("$stderr"_s, _stderr);
    Object->const_set("STDERR"_s, _stderr);

    env->global_set("$/"_s, new StringObject { "\n", 1 });
    GlobalEnv::the()->global_set_write_hook(env, "$/"_s, GlobalVariableAccessHooks::WriteHooks::as_string_or_raise);
    env->global_alias("$-0"_s, "$/"_s);

    env->global_set("$\\"_s, NilObject::the());
    GlobalEnv::the()->global_set_write_hook(env, "$\\"_s, GlobalVariableAccessHooks::WriteHooks::as_string_or_raise);

    env->global_set("$;"_s, NilObject::the());
    env->global_alias("$-F"_s, "$;"_s);

    env->global_set("$DEBUG"_s, FalseObject::the());
    env->global_alias("$-d"_s, "$DEBUG"_s);

    env->global_set("$VERBOSE"_s, FalseObject::the());
    GlobalEnv::the()->global_set_write_hook(env, "$VERBOSE"_s, GlobalVariableAccessHooks::WriteHooks::set_verbose);
    env->global_alias("$-v"_s, "$VERBOSE"_s);
    env->global_alias("$-w"_s, "$VERBOSE"_s);

    GlobalEnv::the()->global_set_read_hook(env, "$$"_s, true, GlobalVariableAccessHooks::ReadHooks::getpid);

    env->global_set("$\""_s, new ArrayObject {}, true);
    env->global_alias("$LOADED_FEATURES"_s, "$\""_s);

    env->global_set("$?"_s, NilObject::the(), true);

    env->global_set("$."_s, Value::integer(0));
    GlobalEnv::the()->global_set_write_hook(env, "$."_s, GlobalVariableAccessHooks::WriteHooks::to_int);

    GlobalEnv::the()->global_set_read_hook(env, "$!"_s, true, GlobalVariableAccessHooks::ReadHooks::last_exception);
    GlobalEnv::the()->global_set_read_hook(env, "$@"_s, true, GlobalVariableAccessHooks::ReadHooks::last_exception_backtrace);

    GlobalEnv::the()->global_set_read_hook(env, "$~"_s, false, GlobalVariableAccessHooks::ReadHooks::last_match);
    GlobalEnv::the()->global_set_write_hook(env, "$~"_s, GlobalVariableAccessHooks::WriteHooks::last_match);
    GlobalEnv::the()->global_set_read_hook(env, "$`"_s, true, GlobalVariableAccessHooks::ReadHooks::last_match_pre_match);
    GlobalEnv::the()->global_set_read_hook(env, "$'"_s, true, GlobalVariableAccessHooks::ReadHooks::last_match_post_match);
    GlobalEnv::the()->global_set_read_hook(env, "$+"_s, true, GlobalVariableAccessHooks::ReadHooks::last_match_last_group);
    GlobalEnv::the()->global_set_read_hook(env, "$&"_s, true, GlobalVariableAccessHooks::ReadHooks::last_match_to_s);

    Value ENV = new Natalie::Object {};
    Object->const_set("ENV"_s, ENV);
    ENV->extend_once(env, Enumerable);

    Value RUBY_VERSION = new StringObject { "3.3.0" };
    Object->const_set("RUBY_VERSION"_s, RUBY_VERSION);

    Value RUBY_COPYRIGHT = new StringObject { "natalie - Copyright (c) 2023 Tim Morgan and contributors" };
    Object->const_set("RUBY_COPYRIGHT"_s, RUBY_COPYRIGHT);

    auto ruby_revision_short = TM::String(ruby_revision, 10);
    StringObject *RUBY_DESCRIPTION = StringObject::format("natalie ({} revision {}) [{}]", ruby_release_date, ruby_revision_short, ruby_platform);
    Object->const_set("RUBY_DESCRIPTION"_s, RUBY_DESCRIPTION);

    Value RUBY_ENGINE = new StringObject { "natalie" };
    Object->const_set("RUBY_ENGINE"_s, RUBY_ENGINE);

    Value RUBY_PATCHLEVEL = new IntegerObject { -1 };
    Object->const_set("RUBY_PATCHLEVEL"_s, RUBY_PATCHLEVEL);

    StringObject *RUBY_PLATFORM = new StringObject { ruby_platform };
    Object->const_set("RUBY_PLATFORM"_s, RUBY_PLATFORM);

    Value RUBY_RELEASE_DATE = new StringObject { ruby_release_date };
    Object->const_set("RUBY_RELEASE_DATE"_s, RUBY_RELEASE_DATE);

    Value RUBY_REVISION = new StringObject { ruby_revision };
    Object->const_set("RUBY_REVISION"_s, RUBY_REVISION);

    ModuleObject *GC = new ModuleObject { "GC" };
    Object->const_set("GC"_s, GC);

    init_bindings(env);

    return env;
}

Value splat(Env *env, Value obj) {
    if (obj->is_array()) {
        return new ArrayObject { *obj->as_array() };
    } else {
        return to_ary(env, obj, false);
    }
}

Value is_case_equal(Env *env, Value case_value, Value when_value, bool is_splat) {
    if (is_splat) {
        if (!when_value->is_array() && when_value->respond_to(env, "to_a"_s)) {
            auto original_class = when_value->klass();
            when_value = when_value->send(env, "to_a"_s);
            if (!when_value->is_array()) {
                env->raise("TypeError", "can't convert {} to Array ({}#to_a gives {})", original_class->inspect_str(), original_class->inspect_str(), when_value->klass()->inspect_str());
            }
        }
        if (when_value->is_array()) {
            for (auto item : *when_value->as_array()) {
                if (item->send(env, "==="_s, { case_value })->is_truthy()) {
                    return TrueObject::the();
                }
            }
            return FalseObject::the();
        }
    }
    return when_value->send(env, "==="_s, { case_value });
}

void run_at_exit_handlers(Env *env) {
    ArrayObject *at_exit_handlers = env->global_get("$NAT_at_exit_handlers"_s)->as_array_or_raise(env);

    Value proc;
    while (!(proc = at_exit_handlers->pop())->is_nil()) {
        if (proc->is_proc())
            NAT_RUN_BLOCK_WITHOUT_BREAK(env, proc->as_proc()->block(), {}, nullptr);
    }
}

void print_exception_with_backtrace(Env *env, ExceptionObject *exception, ThreadObject *thread) {
    std::lock_guard<std::recursive_mutex> lock(g_gc_recursive_mutex);

    auto out = env->global_get("$stderr"_s);

    if (thread) {
        auto formatted = StringObject::format("{} terminated with exception (report_on_exception is true):", thread->inspect_str(env));
        out.send(env, "puts"_s, { formatted });
    }

    if (exception->backtrace()) {
        ArrayObject *backtrace = exception->backtrace()->to_ruby_array();
        if (backtrace->size() > 0) {
            out.send(env, "puts"_s, { new StringObject { "Traceback (most recent call last):" } });
            for (int i = backtrace->size() - 1; i > 0; i--) {
                auto line = backtrace->at(i)->as_string_or_raise(env);
                auto formatted = StringObject::format("        {}: from {}", i, line->string());
                out.send(env, "puts"_s, { formatted });
            }
            auto line = backtrace->at(0)->as_string_or_raise(env);
            auto formatted = StringObject::format("{}: ", line->string());
            out.send(env, "print"_s, { formatted });
        }
    }

    auto formatted = StringObject::format(
        "{} ({})",
        exception->to_s(env)->c_str(),
        exception->klass()->inspect_str());
    out.send(env, "puts"_s, { formatted });
}

void handle_top_level_exception(Env *env, ExceptionObject *exception, bool run_exit_handlers) {
    if (exception->is_a(env, find_top_level_const(env, "SystemExit"_s)->as_class())) {
        Value status_obj = exception->ivar_get(env, "@status"_s);
        if (run_exit_handlers) run_at_exit_handlers(env);
        if (status_obj->type() == Object::Type::Integer) {
            nat_int_t val = status_obj->as_integer()->to_nat_int_t();
            if (val >= 0 && val <= 255) {
                clean_up_and_exit(val);
            } else {
                clean_up_and_exit(1);
            }
        } else {
            clean_up_and_exit(1);
        }
    } else if (exception->is_a(env, find_top_level_const(env, "SignalException"_s)->as_class())) {
        Value signo = exception->ivar_get(env, "@signo"_s);
        if (signo->type() == Object::Type::Integer) {
            nat_int_t val = signo->as_integer()->to_nat_int_t();
            if (val >= 0 && val <= 255) {
                clean_up_and_exit(val + 128);
            } else {
                clean_up_and_exit(1);
            }
        } else {
            clean_up_and_exit(1);
        }
    } else if (ThreadObject::i_am_main()) {
        print_exception_with_backtrace(env, exception);
    } else if (ThreadObject::current()->report_on_exception()) {
        print_exception_with_backtrace(env, exception, ThreadObject::current());
    }
}

ArrayObject *to_ary(Env *env, Value obj, bool raise_for_non_array) {
    if (obj->is_array()) {
        return obj->as_array();
    }

    if (obj->respond_to(env, "to_ary"_s)) {
        auto array = obj.send(env, "to_ary"_s);
        if (!array->is_nil()) {
            if (array->is_array()) {
                return array->as_array();
            } else if (raise_for_non_array) {
                auto class_name = obj->klass()->inspect_str();
                env->raise("TypeError", "can't convert {} to Array ({}#to_ary gives {})", class_name, class_name, array->klass()->inspect_str());
            }
        }
    }

    if (obj->respond_to(env, "to_a"_s)) {
        auto array = obj.send(env, "to_a"_s);
        if (!array->is_nil()) {
            if (array->is_array()) {
                return array->as_array();
            } else if (raise_for_non_array) {
                auto class_name = obj->klass()->inspect_str();
                env->raise("TypeError", "can't convert {} to Array ({}#to_a gives {})", class_name, class_name, array->klass()->inspect_str());
            }
        }
    }

    return new ArrayObject { obj };
}

Value to_ary_for_masgn(Env *env, Value obj) {
    if (obj->is_array())
        return obj->dup(env);

    if (obj->respond_to(env, "to_ary"_s)) {
        auto array = obj.send(env, "to_ary"_s);
        if (array->is_array()) {
            return array->dup(env);
        } else if (!array->is_nil()) {
            auto class_name = obj->klass()->inspect_str();
            env->raise("TypeError", "can't convert {} to Array ({}#to_a gives {})", class_name, class_name, array->klass()->inspect_str());
        }
    }

    return new ArrayObject { obj };
}

void arg_spread(Env *env, Args args, const char *arrangement, ...) {
    va_list va_args;
    va_start(va_args, arrangement);
    size_t len = strlen(arrangement);
    size_t arg_index = 0;
    bool optional = false;
    for (size_t i = 0; i < len; i++) {
        if (arg_index >= args.size() && optional)
            break;
        char c = arrangement[i];
        switch (c) {
        case '|':
            optional = true;
            break;
        case 'o': {
            Object **obj_ptr = va_arg(va_args, Object **); // NOLINT(clang-analyzer-valist.Uninitialized) bug in clang-tidy?
            if (arg_index >= args.size()) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", args.size(), arg_index + 1);
            Object *obj = args[arg_index++].object();
            *obj_ptr = obj;
            break;
        }
        case 'i': {
            int *int_ptr = va_arg(va_args, int *); // NOLINT(clang-analyzer-valist.Uninitialized) bug in clang-tidy?
            if (arg_index >= args.size()) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", args.size(), arg_index + 1);
            Value obj = args[arg_index++];
            obj->assert_type(env, Object::Type::Integer, "Integer");
            *int_ptr = obj->as_integer()->to_nat_int_t();
            break;
        }
        case 's': {
            const char **str_ptr = va_arg(va_args, const char **); // NOLINT(clang-analyzer-valist.Uninitialized) bug in clang-tidy?
            if (arg_index >= args.size()) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", args.size(), arg_index + 1);
            Value obj = args[arg_index++];
            if (obj == NilObject::the()) {
                *str_ptr = nullptr;
            } else {
                obj->assert_type(env, Object::Type::String, "String");
            }
            *str_ptr = obj->as_string()->c_str();
            break;
        }
        case 'b': {
            bool *bool_ptr = va_arg(va_args, bool *); // NOLINT(clang-analyzer-valist.Uninitialized) bug in clang-tidy?
            if (arg_index >= args.size()) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", args.size(), arg_index + 1);
            Value obj = args[arg_index++];
            *bool_ptr = obj->is_truthy();
            break;
        }
        case 'v': {
            void **void_ptr = va_arg(va_args, void **); // NOLINT(clang-analyzer-valist.Uninitialized) bug in clang-tidy?
            if (arg_index >= args.size()) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", args.size(), arg_index + 1);
            Value obj = args[arg_index++];
            obj = obj->ivar_get(env, "@_ptr"_s);
            assert(obj->type() == Object::Type::VoidP);
            *void_ptr = obj->as_void_p()->void_ptr();
            break;
        }
        default:
            fprintf(stderr, "Unknown arg spread arrangement specifier: %%%c", c);
            abort();
        }
    }
    va_end(va_args);
}

std::pair<Value, Value> coerce(Env *env, Value lhs, Value rhs, CoerceInvalidReturnValueMode invalid_return_value_mode) {
    auto coerce_symbol = "coerce"_s;
    if (lhs->respond_to(env, coerce_symbol)) {
        if (lhs->is_synthesized())
            lhs = lhs->dup(env);
        if (rhs->is_synthesized())
            rhs = rhs->dup(env);
        Value coerced = lhs.send(env, coerce_symbol, { rhs });
        if (!coerced->is_array()) {
            if (invalid_return_value_mode == CoerceInvalidReturnValueMode::Raise)
                env->raise("TypeError", "coerce must return [x, y]");
            else
                return { rhs, lhs };
        }
        lhs = (*coerced->as_array())[0];
        rhs = (*coerced->as_array())[1];
        return { lhs, rhs };
    } else {
        return { rhs, lhs };
    }
}

Block *to_block(Env *env, Value proc_or_nil) {
    if (proc_or_nil->is_nil()) {
        return nullptr;
    }
    return proc_or_nil->to_proc(env)->block();
}

#define NAT_SHELL_READ_BYTES 1024

Value shell_backticks(Env *env, Value command) {
    command->assert_type(env, Object::Type::String, "String");
    int pid;
    auto process = popen2(command->as_string()->c_str(), "r", pid);
    if (!process)
        env->raise_errno();
    char buf[NAT_SHELL_READ_BYTES];
    auto result = fgets(buf, NAT_SHELL_READ_BYTES, process);
    StringObject *out;
    if (result) {
        out = new StringObject { buf };
        while (1) {
            result = fgets(buf, NAT_SHELL_READ_BYTES, process);
            if (!result) break;
            out->append(buf);
        }
    } else {
        out = new StringObject {};
    }
    int status = pclose2(process, pid);
    set_status_object(env, pid, status);
    return out;
}

// https://stackoverflow.com/a/26852210/197498
FILE *popen2(const char *command, const char *type, int &pid) {
    pid_t child_pid;
    int fd[2];
    if (pipe(fd) != 0) {
        fprintf(stderr, "Unable to open pipe (errno=%d)", errno);
        abort();
    }

    const int read = 0;
    const int write = 1;

    bool is_read = strcmp(type, "r") == 0;

    if ((child_pid = fork()) == -1) {
        perror("fork");
        clean_up_and_exit(1);
    }

    // child process
    if (child_pid == 0) {
        if (is_read) {
            close(fd[read]); // close the READ end of the pipe since the child's fd is write-only
            dup2(fd[write], 1); // redirect stdout to pipe
        } else {
            close(fd[write]); // close the WRITE end of the pipe since the child's fd is read-only
            dup2(fd[read], 0); // redirect stdin to pipe
        }

        setpgid(child_pid, child_pid); // needed so negative PIDs can kill children of /bin/sh
        execl("/bin/sh", "/bin/sh", "-c", command, NULL);
        clean_up_and_exit(0);
    } else {
        if (is_read) {
            close(fd[write]); // close the WRITE end of the pipe since parent's fd is read-only
        } else {
            close(fd[read]); // close the READ end of the pipe since parent's fd is write-only
        }
    }

    pid = child_pid;

    if (is_read)
        return fdopen(fd[read], "r");

    return fdopen(fd[write], "w");
}

// https://stackoverflow.com/a/26852210/197498
int pclose2(FILE *fp, pid_t pid) {
    int stat;

    fclose(fp);
    while (waitpid(pid, &stat, 0) == -1) {
        if (errno != EINTR) {
            stat = -1;
            break;
        }
    }

    return stat;
}

void set_status_object(Env *env, int pid, int status) {
    auto status_obj = GlobalEnv::the()->Object()->const_fetch("Process"_s)->const_fetch("Status"_s).send(env, "new"_s);
    status_obj->ivar_set(env, "@to_i"_s, Value::integer(status));
    status_obj->ivar_set(env, "@exitstatus"_s, Value::integer(WEXITSTATUS(status)));
    status_obj->ivar_set(env, "@pid"_s, Value::integer(pid));
    env->global_set("$?"_s, status_obj, true);
}

Value super(Env *env, Value self, Args args, Block *block) {
    auto current_method = env->current_method();
    auto klass = self->singleton_class();
    if (!klass)
        klass = self->klass();
    auto super_method = klass->find_method(env, SymbolObject::intern(current_method->name()), current_method);
    if (!super_method.is_defined()) {
        if (self->is_module()) {
            env->raise("NoMethodError", "super: no superclass method `{}' for {}:{}", current_method->name(), self->as_module()->inspect_str(), self->klass()->inspect_str());
        } else {
            env->raise("NoMethodError", "super: no superclass method `{}' for {}", current_method->name(), self->inspect_str(env));
        }
    }
    assert(super_method.method() != current_method);
    args.pop_empty_keyword_hash();
    return super_method.method()->call(env, self, args, block);
}

void clean_up_and_exit(int status) {
    ThreadObject::detach_all();
    if (Heap::the().collect_all_at_exit()) {
        delete &Heap::the();
        delete NativeProfiler::the();
    }
    exit(status);
}

Value bool_object(bool b) {
    if (b)
        return TrueObject::the();
    else
        return FalseObject::the();
}

int hex_char_to_decimal_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else {
        return -1;
    }
};

int pipe2(int pipefd[2], int flags) {
    if (flags & (~O_NONBLOCK & ~O_CLOEXEC))
        NAT_NOT_YET_IMPLEMENTED("Extra flags passed to our pipe2 that aren't supported.");

#ifdef __APPLE__
    if (pipe(pipefd) == -1)
        return -1;

    if (flags & O_NONBLOCK) {
        if (fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(pipefd[1], F_SETFL, O_NONBLOCK) == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
        }
    }

    if (flags & O_CLOEXEC) {
        if (fcntl(pipefd[0], F_SETFD, O_CLOEXEC) == -1 || fcntl(pipefd[1], F_SETFD, O_CLOEXEC) == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
        }
    }

    return 0;
#else
    return ::pipe2(pipefd, flags);
#endif
}

void trap_signal(int signal, void (*handler)(int, siginfo_t *, void *)) {
    struct sigaction sa;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(signal, &sa, nullptr) == -1) {
        printf("Failed to trap %d\n", signal);
        exit(1);
    }
}

void gc_signal_handler(int signal, siginfo_t *, void *ucontext) {
    auto thread = ThreadObject::current();
    if (!thread || thread->is_main()) return;

    switch (signal) {
    case SIGUSR1: {
        auto ctx = thread->get_context();
        if (!ctx) {
            char msg[] = "Fatal: Could not get pointer for thread context.";
            assert(::write(STDERR_FILENO, msg, sizeof(msg)) != -1);
            abort();
        }
        memcpy(ctx, ucontext, sizeof(ucontext_t));
        thread->set_suspend_status(ThreadObject::SuspendStatus::Suspended);
#ifdef NAT_DEBUG_THREADS
        char msg[] = "THREAD DEBUG: Thread suspended\n";
        assert(::write(STDERR_FILENO, msg, sizeof(msg)) != -1);
#endif
        pause();
        thread->set_suspend_status(ThreadObject::SuspendStatus::Running);
        break;
    }
    case SIGUSR2:
#ifdef NAT_DEBUG_THREADS
        char msg2[] = "THREAD DEBUG: Thread resumed\n";
        assert(::write(STDERR_FILENO, msg2, sizeof(msg2)) != -1);
#endif
        thread->set_suspend_status(ThreadObject::SuspendStatus::Running);
        break;
    }
}

void sigint_handler(int, siginfo_t *, void *) {
    const char *msg = "Interrupt\n";
    auto bytes_written = write(STDOUT_FILENO, msg, strlen(msg));
    if (bytes_written == -1) abort();
    exit(128 + SIGINT);
}

void sigpipe_handler(int, siginfo_t *, void *) {
    // TODO: do something here?
}

}
