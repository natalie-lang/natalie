#include "natalie.hpp"
#include "natalie/forward.hpp"
#include "natalie/value_ptr.hpp"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <sys/wait.h>

namespace Natalie {

Env *build_top_env() {
    auto *global_env = GlobalEnv::the();
    auto *env = new Env {};
    global_env->set_main_env(env);

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

    ClassValue *Binding = Object->subclass(env, "Binding", Value::Type::Binding);
    global_env->set_Binding(Binding);
    Object->const_set(SymbolValue::intern("Binding"), Binding);

    ClassValue *Hash = Object->subclass(env, "Hash", Value::Type::Hash);
    global_env->set_Hash(Hash);
    Object->const_set(SymbolValue::intern("Hash"), Hash);
    Hash->include_once(env, Enumerable);

    ClassValue *Random = Object->subclass(env, "Random", Value::Type::Random);
    global_env->set_Random(Random);
    Object->const_set(SymbolValue::intern("Random"), Random);
    Random->const_set(SymbolValue::intern("DEFAULT"), (new RandomValue)->initialize(env, nullptr));

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
    ClassValue *ScriptError = Exception->subclass(env, "ScriptError", Value::Type::Exception);
    Object->const_set(SymbolValue::intern("ScriptError"), ScriptError);
    ClassValue *SyntaxError = ScriptError->subclass(env, "SyntaxError", Value::Type::Exception);
    Object->const_set(SymbolValue::intern("SyntaxError"), SyntaxError);
    ClassValue *StandardError = Exception->subclass(env, "StandardError", Value::Type::Exception);
    Object->const_set(SymbolValue::intern("StandardError"), StandardError);
    ClassValue *NameError = StandardError->subclass(env, "NameError", Value::Type::Exception);
    Object->const_set(SymbolValue::intern("NameError"), NameError);

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

    Value *main_obj = new Value {};
    main_obj->add_main_object_flag();
    GlobalEnv::the()->set_main_obj(main_obj);

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
    Object->const_set(SymbolValue::intern("Sexp"), Sexp);

    ValuePtr RUBY_VERSION = new StringValue { "3.0.0" };
    Object->const_set(SymbolValue::intern("RUBY_VERSION"), RUBY_VERSION);

    ValuePtr RUBY_ENGINE = new StringValue { "natalie" };
    Object->const_set(SymbolValue::intern("RUBY_ENGINE"), RUBY_ENGINE);

    StringValue *RUBY_PLATFORM = new StringValue { ruby_platform };
    Object->const_set(SymbolValue::intern("RUBY_PLATFORM"), RUBY_PLATFORM);

    ModuleValue *GC = new ModuleValue { "GC" };
    Object->const_set(SymbolValue::intern("GC"), GC);

    init_bindings(env);

    return env;
}

ValuePtr splat(Env *env, ValuePtr obj) {
    if (obj->is_array()) {
        return new ArrayValue { *obj->as_array() };
    } else {
        return to_ary(env, obj, false);
    }
}

void run_at_exit_handlers(Env *env) {
    ArrayValue *at_exit_handlers = env->global_get(SymbolValue::intern("$NAT_at_exit_handlers"))->as_array();
    assert(at_exit_handlers);
    for (int i = at_exit_handlers->size() - 1; i >= 0; i--) {
        ValuePtr proc = (*at_exit_handlers)[i];
        assert(proc);
        assert(proc->is_proc());
        NAT_RUN_BLOCK_WITHOUT_BREAK(env, proc->as_proc()->block(), 0, nullptr, nullptr);
    }
}

void print_exception_with_backtrace(Env *env, ExceptionValue *exception) {
    IoValue *_stderr = env->global_get(SymbolValue::intern("$stderr"))->as_io();
    int fd = _stderr->fileno();
    const ArrayValue *backtrace = exception->backtrace();
    if (backtrace && backtrace->size() > 0) {
        dprintf(fd, "Traceback (most recent call last):\n");
        for (int i = backtrace->size() - 1; i > 0; i--) {
            StringValue *line = (*backtrace)[i]->as_string();
            assert(line->type() == Value::Type::String);
            dprintf(fd, "        %d: from %s\n", i, line->c_str());
        }
        StringValue *line = (*backtrace)[0]->as_string();
        dprintf(fd, "%s: ", line->c_str());
    }
    dprintf(fd, "%s (%s)\n", exception->message()->c_str(), exception->klass()->class_name_or_blank()->c_str());
}

void handle_top_level_exception(Env *env, ExceptionValue *exception, bool run_exit_handlers) {
    if (exception->is_a(env, GlobalEnv::the()->Object()->const_find(env, SymbolValue::intern("SystemExit"))->as_class())) {
        ValuePtr status_obj = exception->ivar_get(env, SymbolValue::intern("@status"));
        if (run_exit_handlers) run_at_exit_handlers(env);
        if (status_obj->type() == Value::Type::Integer) {
            nat_int_t val = status_obj->as_integer()->to_nat_int_t();
            if (val >= 0 && val <= 255) {
                clean_up_and_exit(val);
            } else {
                clean_up_and_exit(1);
            }
        } else {
            clean_up_and_exit(1);
        }
    } else {
        print_exception_with_backtrace(env, exception);
    }
}

ArrayValue *to_ary(Env *env, ValuePtr obj, bool raise_for_non_array) {
    auto to_ary_symbol = SymbolValue::intern("to_ary");
    if (obj->is_array()) {
        return obj->as_array();
    } else if (obj->respond_to(env, to_ary_symbol)) {
        ValuePtr ary = obj.send(env, to_ary_symbol);
        if (ary->is_array()) {
            return ary->as_array();
        } else if (ary->is_nil() || !raise_for_non_array) {
            ary = new ArrayValue {};
            ary->as_array()->push(obj);
            return ary->as_array();
        } else {
            auto *class_name = obj->klass()->class_name_or_blank();
            env->raise("TypeError", "can't convert {} to Array ({}#to_ary gives {})", class_name, class_name, ary->klass()->class_name_or_blank());
        }
    } else {
        ArrayValue *ary = new ArrayValue {};
        ary->push(obj);
        return ary;
    }
}

static ValuePtr splat_value(Env *env, ValuePtr value, size_t index, size_t offset_from_end) {
    ArrayValue *splat = new ArrayValue {};
    if (value->is_array() && index < value->as_array()->size() - offset_from_end) {
        for (size_t s = index; s < value->as_array()->size() - offset_from_end; s++) {
            splat->push((*value->as_array())[s]);
        }
    }
    return splat;
}

ValuePtr arg_value_by_path(Env *env, ValuePtr value, ValuePtr default_value, bool splat, bool has_kwargs, int total_count, int default_count, bool defaults_on_right, int offset_from_end, size_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    bool has_default = !!default_value;
    if (!default_value) default_value = NilValue::the();
    bool defaults_on_left = !defaults_on_right;
    int required_count = total_count - default_count; // 0
    ValuePtr return_value = value;
    for (size_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);

        if (splat && i == path_size - 1) {
            va_end(args);
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {
                assert(return_value->as_array()->size() <= NAT_INT_MAX);
                nat_int_t ary_len = return_value->as_array()->size();

                int first_required = default_count;
                int remain = ary_len - required_count;

                if (has_default && index >= remain && index < first_required && defaults_on_left) {
                    // this is an arg with a default value;
                    // not enough values to fill all the required args and this one
                    va_end(args);
                    return default_value;
                }

                if (i == 0 && path_size == 1) {
                    // shift index left if needed
                    int extra_count = ary_len - required_count;
                    if (defaults_on_left && extra_count > 0 && default_count >= extra_count && index >= extra_count) {
                        index -= (default_count - extra_count);
                    } else if (ary_len <= required_count && defaults_on_left) {
                        index -= (default_count);
                    }
                }

                if (index < 0) {
                    // negative offset index should go from the right
                    if (ary_len >= total_count) {
                        index = ary_len + index;
                    } else {
                        // not enough values to fill from the right
                        // also, assume there is a splat prior to this index
                        index = total_count + index;
                    }
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = (*return_value->as_array())[index];

                    if (has_default && has_kwargs && i == path_size - 1) {
                        if (return_value->is_hash()) {
                            return_value = default_value;
                        }
                    }

                } else {
                    // index past the end of the array, so use default
                    return_value = default_value;
                }

            } else if (index == 0) {
                // not an array, so nothing to do (the object itself is returned)
                // no-op

            } else {
                // not an array, and index isn't zero
                return_value = default_value;
            }
        }
    }
    va_end(args);
    return return_value;
}

ValuePtr array_value_by_path(Env *env, ValuePtr value, ValuePtr default_value, bool splat, int offset_from_end, size_t path_size, ...) {
    va_list args;
    va_start(args, path_size);
    ValuePtr return_value = value;
    for (size_t i = 0; i < path_size; i++) {
        int index = va_arg(args, int);
        if (splat && i == path_size - 1) {
            va_end(args);
            return splat_value(env, return_value, index, offset_from_end);
        } else {
            if (return_value->is_array()) {

                assert(return_value->as_array()->size() <= NAT_INT_MAX);
                nat_int_t ary_len = return_value->as_array()->size();

                if (index < 0) {
                    // negative offset index should go from the right
                    index = ary_len + index;
                }

                if (index < 0) {
                    // not enough values in the array, so use default
                    return_value = default_value;

                } else if (index < ary_len) {
                    // value available, yay!
                    return_value = (*return_value->as_array())[index];

                } else {
                    // index past the end of the array, so use default
                    return_value = default_value;
                }

            } else if (index == 0) {
                // not an array, so nothing to do (the object itself is returned)
                // no-op

            } else {
                // not an array, and index isn't zero
                return_value = default_value;
            }
        }
    }
    va_end(args);
    return return_value;
}

ValuePtr kwarg_value_by_name(Env *env, ValuePtr args, const char *name, ValuePtr default_value) {
    return kwarg_value_by_name(env, args->as_array(), name, default_value);
}

HashValue *kwarg_hash(ValuePtr args) {
    return kwarg_hash(args->as_array());
}

HashValue *kwarg_hash(ArrayValue *args) {
    HashValue *hash;
    if (args->size() == 0) {
        hash = new HashValue {};
    } else {
        auto maybe_hash = (*args)[args->size() - 1];
        if (maybe_hash->is_hash())
            hash = maybe_hash->as_hash();
        else
            hash = new HashValue {};
    }
    return hash;
}

ValuePtr kwarg_value_by_name(Env *env, ArrayValue *args, const char *name, ValuePtr default_value) {
    auto hash = kwarg_hash(args);
    ValuePtr value = hash->as_hash()->remove(env, SymbolValue::intern(name));
    if (!value) {
        if (default_value) {
            return default_value;
        } else {
            env->raise("ArgumentError", "missing keyword: :{}", name);
        }
    }
    return value;
}

ArrayValue *args_to_array(Env *env, size_t argc, ValuePtr *args) {
    ArrayValue *ary = new ArrayValue {};
    for (size_t i = 0; i < argc; i++) {
        ary->push(args[i]);
    }
    return ary;
}

// much like args_to_array above, but when a block is given a single arg,
// and the block wants multiple args, call to_ary on the first arg and return that
ArrayValue *block_args_to_array(Env *env, size_t signature_size, size_t argc, ValuePtr *args) {
    if (argc == 1 && signature_size > 1) {
        return to_ary(env, args[0], true);
    }
    return args_to_array(env, argc, args);
}

void arg_spread(Env *env, size_t argc, ValuePtr *args, const char *arrangement, ...) {
    va_list va_args;
    va_start(va_args, arrangement);
    size_t len = strlen(arrangement);
    size_t arg_index = 0;
    bool optional = false;
    for (size_t i = 0; i < len; i++) {
        if (arg_index >= argc && optional)
            break;
        char c = arrangement[i];
        switch (c) {
        case '|':
            optional = true;
            break;
        case 'o': {
            Value **obj_ptr = va_arg(va_args, Value **);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            Value *obj = args[arg_index++].value();
            *obj_ptr = obj;
            break;
        }
        case 'i': {
            int *int_ptr = va_arg(va_args, int *);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            obj->assert_type(env, Value::Type::Integer, "Integer");
            *int_ptr = obj->as_integer()->to_nat_int_t();
            break;
        }
        case 's': {
            const char **str_ptr = va_arg(va_args, const char **);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            if (obj == NilValue::the()) {
                *str_ptr = nullptr;
            } else {
                obj->assert_type(env, Value::Type::String, "String");
            }
            *str_ptr = obj->as_string()->c_str();
            break;
        }
        case 'b': {
            bool *bool_ptr = va_arg(va_args, bool *);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            *bool_ptr = obj->is_truthy();
            break;
        }
        case 'v': {
            void **void_ptr = va_arg(va_args, void **);
            if (arg_index >= argc) env->raise("ArgumentError", "wrong number of arguments (given {}, expected {})", argc, arg_index + 1);
            ValuePtr obj = args[arg_index++];
            obj = obj->ivar_get(env, SymbolValue::intern("@_ptr"));
            assert(obj->type() == Value::Type::VoidP);
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

std::pair<ValuePtr, ValuePtr> coerce(Env *env, ValuePtr lhs, ValuePtr rhs) {
    auto coerce_symbol = SymbolValue::intern("coerce");
    if (lhs->respond_to(env, coerce_symbol)) {
        ValuePtr coerced = lhs.send(env, coerce_symbol, { rhs });
        if (!coerced->is_array()) {
            env->raise("TypeError", "coerce must return [x, y]");
        }
        lhs = (*coerced->as_array())[0];
        rhs = (*coerced->as_array())[1];
        return { lhs, rhs };
    } else {
        return { rhs, lhs };
    }
}

char *zero_string(int size) {
    char *buf = new char[size + 1];
    memset(buf, '0', size);
    buf[size] = 0;
    return buf;
}

Block *proc_to_block_arg(Env *env, ValuePtr proc_or_nil) {
    if (proc_or_nil->is_nil()) {
        return nullptr;
    }
    return proc_or_nil->to_proc(env)->block();
}

#define NAT_SHELL_READ_BYTES 1024

ValuePtr shell_backticks(Env *env, ValuePtr command) {
    command->assert_type(env, Value::Type::String, "String");
    int pid;
    auto process = popen2(command->as_string()->c_str(), "r", pid);
    if (!process)
        env->raise_errno();
    char buf[NAT_SHELL_READ_BYTES];
    auto result = fgets(buf, NAT_SHELL_READ_BYTES, process);
    StringValue *out;
    if (result) {
        out = new StringValue { buf };
        while (1) {
            result = fgets(buf, NAT_SHELL_READ_BYTES, process);
            if (!result) break;
            out->append(env, buf);
        }
    } else {
        out = new StringValue {};
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
    auto status_obj = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Process"))->const_fetch(SymbolValue::intern("Status")).send(env, SymbolValue::intern("new"));
    status_obj->ivar_set(env, SymbolValue::intern("@to_i"), ValuePtr::integer(status));
    status_obj->ivar_set(env, SymbolValue::intern("@exitstatus"), ValuePtr::integer(WEXITSTATUS(status)));
    status_obj->ivar_set(env, SymbolValue::intern("@pid"), ValuePtr::integer(pid));
    env->global_set(SymbolValue::intern("$?"), status_obj);
}

const String *int_to_hex_string(nat_int_t num, bool capitalize) {
    if (num == 0) {
        return new String("0");
    } else {
        char buf[100]; // ought to be enough for anybody ;-)
        if (capitalize) {
            snprintf(buf, 100, "0X%llX", num);
        } else {
            snprintf(buf, 100, "0x%llx", num);
        }
        return new String(buf);
    }
}

ValuePtr super(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
    auto current_method = env->current_method();
    auto super_method = self->klass()->find_method(env, SymbolValue::intern(current_method->name()), nullptr, current_method);
    if (!super_method)
        env->raise("NoMethodError", "super: no superclass method `{}' for {}", current_method->name(), self->inspect_str(env));
    assert(super_method != current_method);
    return super_method->call(env, self, argc, args, block);
}

void clean_up_and_exit(int status) {
    if (Heap::the().collect_all_at_exit())
        delete &Heap::the();
    exit(status);
}

}
