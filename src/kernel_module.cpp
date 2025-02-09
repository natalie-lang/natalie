#include "natalie.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/thread_object.hpp"
#include "natalie/throw_catch_exception.hpp"

#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

extern char **environ;

namespace Natalie {

Value KernelModule::Array(Env *env, Value value) {
    return Natalie::to_ary(env, value, true);
}

Value KernelModule::abort_method(Env *env, Value message) {
    auto SystemExit = GlobalEnv::the()->Object()->const_fetch("SystemExit"_s);
    ExceptionObject *exception;

    if (message) {
        if (!message.is_string())
            message = message.to_str(env);

        message.assert_type(env, Object::Type::String, "String");

        exception = SystemExit.send(env, "new"_s, { Value::integer(1), message })->as_exception();

        auto out = env->global_get("$stderr"_s);
        out.send(env, "puts"_s, { message });
    } else {
        exception = SystemExit.send(env, "new"_s, { Value::integer(1) })->as_exception();
    }

    env->raise_exception(exception);

    return NilObject::the();
}

Value KernelModule::at_exit(Env *env, Block *block) {
    ArrayObject *at_exit_handlers = env->global_get("$NAT_at_exit_handlers"_s)->as_array();
    env->ensure_block_given(block);
    Value proc = new ProcObject { block };
    at_exit_handlers->push(proc);
    return proc;
}

Value KernelModule::backtick(Env *env, Value command) {
    constexpr size_t NAT_SHELL_READ_BYTES = 1024;
    pid_t pid;
    auto process = popen2(command.to_str(env)->c_str(), "r", pid);
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

Value KernelModule::binding(Env *env) {
    return new BindingObject { env };
}

Value KernelModule::caller(Env *env, Value start, Value length) {
    if (!start)
        start = Value::integer(1); // remove the frame for the call site of Kernel#caller
    auto backtrace = env->backtrace();
    auto ary = backtrace->to_ruby_array();
    ary->shift(); // remove the frame for Kernel#caller itself
    if (start && start.is_range()) {
        ary = ary->ref(env, start)->as_array();
    } else {
        ary->shift(env, start);
        if (length)
            ary = ary->first(env, length)->as_array();
    }
    return ary;
}

Value KernelModule::caller_locations(Env *env, Value start, Value length) {
    if (!start)
        start = Value::integer(1); // remove the frame for the call site of Kernel#caller_locations
    auto backtrace = env->backtrace();
    auto ary = backtrace->to_ruby_backtrace_locations_array();
    ary->shift(); // remove the frame for Kernel#caller_locations itself
    if (start && start.is_range()) {
        ary = ary->ref(env, start)->as_array();
    } else {
        ary->shift(env, start);
        if (length)
            ary = ary->first(env, length)->as_array();
    }
    return ary;
}

Value KernelModule::catch_method(Env *env, Value name, Block *block) {
    if (!block)
        env->raise("LocalJumpError", "no block given");
    if (!name) name = new Object {};

    try {
        Env block_env { env };
        block_env.set_catch(name);
        return block->run(&block_env, { name }, nullptr);
    } catch (ThrowCatchException *e) {
        if (Object::equal(e->get_name(), name))
            return e->get_value();
        else
            throw e;
    }
}

Value KernelModule::Complex(Env *env, Value real, Value imaginary, Value exception) {
    return Complex(env, real, imaginary, exception ? exception.is_true() : true);
}

Value KernelModule::Complex(Env *env, Value real, Value imaginary, bool exception) {
    if (real.is_string())
        return Complex(env, real->as_string(), imaginary, exception);

    if (real.is_complex() && imaginary == nullptr) {
        return real;
    } else if (real.is_complex() && imaginary.is_complex()) {
        auto new_real = real->as_complex()->real().send(env, "-"_s, { imaginary->as_complex()->imaginary() });
        auto new_imaginary = real->as_complex()->imaginary().send(env, "+"_s, { imaginary->as_complex()->real() });
        return new ComplexObject { new_real, new_imaginary };
    } else if (imaginary == nullptr) {
        return new ComplexObject { real };
    } else {
        return new ComplexObject { real, imaginary };
    }

    if (exception)
        env->raise("TypeError", "can't convert {} into Complex", real.klass()->inspect_str());
    else
        return nullptr;
}

Value KernelModule::Complex(Env *env, StringObject *real, Value imaginary, bool exception) {
    auto error = [&]() -> Value {
        if (exception)
            env->raise("ArgumentError", "invalid value for convert(): \"{}\"", real->string());
        return NilObject::the();
    };
    if (!real->is_ascii_only())
        return error();
    enum class State {
        Start,
        RealInteger,
        Fallback, // In case of an error, use String#to_c until we finalize this parser
    };
    auto state = State::Start;
    const char *real_start = nullptr;
    const char *real_end = nullptr;
    for (const char *c = real->c_str(); c < real->c_str() + real->bytesize(); c++) {
        if (*c == 0) {
            if (exception)
                env->raise("ArgumentError", "string contains null byte");
            return NilObject::the();
        }
        switch (state) {
        case State::Start:
            if ((*c >= '0' && *c <= '9') || *c == '+' || *c == '-') {
                real_start = real_end = c;
                state = State::RealInteger;
            } else {
                state = State::Fallback;
            }
            break;
        case State::RealInteger:
            if (*c >= '0' && *c <= '9') {
                real_end = c;
            } else {
                state = State::Fallback;
            }
            break;
        case State::Fallback:
            break;
        }
    }

    switch (state) {
    case State::Fallback:
        return real->send(env, "to_c"_s);
    case State::Start:
        return error();
    default: {
        if (real_start == nullptr || real_end == nullptr)
            return error();
        auto new_real = new StringObject { real_start, static_cast<size_t>(real_end - real_start + 1) };
        return new ComplexObject { Integer(env, new_real) };
    }
    }
}

Value KernelModule::cur_callee(Env *env) {
    auto method = env->caller()->current_method();
    if (method)
        return SymbolObject::intern(method->name());

    return NilObject::the();
}

Value KernelModule::cur_dir(Env *env) {
    if (env->file() == nullptr) {
        env->raise("RuntimeError", "could not get current directory");
    } else if (strcmp(env->file(), "-e") == 0) {
        return new StringObject { "." };
    } else {
        Value relative = new StringObject { env->file() };
        StringObject *absolute = FileObject::expand_path(env, relative, nullptr)->as_string();
        size_t last_slash = 0;
        bool found = false;
        for (size_t i = 0; i < absolute->length(); i++) {
            if (absolute->c_str()[i] == '/') {
                found = true;
                last_slash = i;
            }
        }
        if (found)
            absolute->truncate(last_slash);
        return absolute;
    }
}

Value KernelModule::exit(Env *env, Value status) {
    if (!status || status.is_true()) {
        status = Value::integer(0);
    } else if (status.is_false()) {
        status = Value::integer(1);
    } else if (status.is_integer()) {
        // use status passed in
    }

    ExceptionObject *exception = new ExceptionObject { find_top_level_const(env, "SystemExit"_s)->as_class(), new StringObject { "exit" } };
    exception->ivar_set(env, "@status"_s, status.to_int(env));
    env->raise_exception(exception);
    return NilObject::the();
}

Value KernelModule::exit_bang(Env *env, Value status) {
    env->global_get("$NAT_at_exit_handlers"_s)->as_array_or_raise(env)->clear(env);
    return exit(env, status);
}

Value KernelModule::Integer(Env *env, Value value, Value base, Value exception) {
    nat_int_t base_int = 0; // default to zero if unset
    if (base)
        base_int = base.to_int(env).to_nat_int_t();
    return Integer(env, value, base_int, exception ? exception.is_true() : true);
}

Value KernelModule::Integer(Env *env, Value value, nat_int_t base, bool exception) {
    if (value.is_string()) {
        auto result = value->as_string()->convert_integer(env, base);
        if (!result && exception) {
            env->raise("ArgumentError", "invalid value for Integer(): {}", value.inspect_str(env));
        }
        return result;
    }
    // base can only be given for string values
    if (base)
        env->raise("ArgumentError", "Cannot give base for non-string value");

    // return Integer as-is
    if (value.is_integer())
        return Value(value);

    // Infinity/NaN cannot be converted to Integer
    if (value.is_float()) {
        auto float_obj = value->as_float();
        if (float_obj->is_nan() || float_obj->is_infinity()) {
            if (exception)
                env->raise("FloatDomainError", "{}", float_obj->to_s());
            else
                return Value(NilObject::the());
        }
    }

    if (!value.is_nil()) {
        // Try using to_int to coerce to an Integer
        if (value.respond_to(env, "to_int"_s)) {
            auto result = value.send(env, "to_int"_s);
            if (result.is_integer()) return result;
        }
        // If to_int doesn't exist or doesn't return an Integer, try to_i instead.
        if (value.respond_to(env, "to_i"_s)) {
            auto result = value.send(env, "to_i"_s);
            if (result.is_integer()) return result;
        }
    }
    if (exception)
        env->raise("TypeError", "can't convert {} into Integer", value.klass()->inspect_str());
    else
        return Value(NilObject::the());
}

Value KernelModule::Float(Env *env, Value value, Value exception) {
    return Float(env, value, exception ? exception.is_true() : true);
}

Value KernelModule::Float(Env *env, Value value, bool exception) {
    if (value.is_float()) {
        return value;
    } else if (value.is_string()) {
        auto result = value->as_string()->convert_float();
        if (!result && exception) {
            env->raise("ArgumentError", "invalid value for Float(): {}", value.inspect_str(env));
        }
        return result;
    } else if (!value.is_nil() && value.respond_to(env, "to_f"_s)) {
        auto result = value.send(env, "to_f"_s);
        if (result.is_float()) {
            return result;
        }
    }
    if (exception)
        env->raise("TypeError", "can't convert {} into Float", value.klass()->inspect_str());
    else
        return nullptr;
}

Value KernelModule::fork(Env *env, Block *block) {
    auto pid = ::fork();

    if (pid == -1)
        env->raise_errno();

    if (block) {
        if (pid == 0) {
            // child
            block->run(env, { Value::integer(pid) }, nullptr);
            ::exit(0);
        } else {
            // parent
            return Value::integer(pid);
        }
    } else {
        if (pid == 0) {
            // child
            return NilObject::the();
        } else {
            // parent
            return Value::integer(pid);
        }
    }
}

Value KernelModule::gets(Env *env) {
    char buf[2048];
    if (!fgets(buf, 2048, stdin))
        return NilObject::the();
    return new StringObject { buf };
}

Value KernelModule::get_usage(Env *env) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        return NilObject::the();

    HashObject *hash = new HashObject {};
    hash->put(env, new StringObject { "maxrss" }, Value::integer(usage.ru_maxrss));
    hash->put(env, new StringObject { "ixrss" }, Value::integer(usage.ru_ixrss));
    hash->put(env, new StringObject { "idrss" }, Value::integer(usage.ru_idrss));
    hash->put(env, new StringObject { "isrss" }, Value::integer(usage.ru_isrss));
    hash->put(env, new StringObject { "minflt" }, Value::integer(usage.ru_minflt));
    hash->put(env, new StringObject { "majflt" }, Value::integer(usage.ru_majflt));
    hash->put(env, new StringObject { "nswap" }, Value::integer(usage.ru_nswap));
    hash->put(env, new StringObject { "inblock" }, Value::integer(usage.ru_inblock));
    hash->put(env, new StringObject { "oublock" }, Value::integer(usage.ru_oublock));
    hash->put(env, new StringObject { "msgsnd" }, Value::integer(usage.ru_msgsnd));
    hash->put(env, new StringObject { "msgrcv" }, Value::integer(usage.ru_msgrcv));
    hash->put(env, new StringObject { "nsignals" }, Value::integer(usage.ru_nsignals));
    hash->put(env, new StringObject { "nvcsw" }, Value::integer(usage.ru_nvcsw));
    hash->put(env, new StringObject { "nivcsw" }, Value::integer(usage.ru_nivcsw));
    return hash;
}

Value KernelModule::global_variables(Env *env) {
    return GlobalEnv::the()->global_list(env);
}

Value KernelModule::Hash(Env *env, Value value) {
    if (value.is_hash())
        return value;

    if (value.is_nil() || (value.is_array() && value->as_array()->is_empty()))
        return new HashObject;

    return value.to_hash(env);
}

Value KernelModule::lambda(Env *env, Block *block) {
    if (block) {
        block->set_type(Block::BlockType::Lambda);
        return new ProcObject { block };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

Value KernelModule::p(Env *env, Args &&args) {
    if (args.size() == 0) {
        return NilObject::the();
    } else if (args.size() == 1) {
        Value arg = args[0].send(env, "inspect"_s);
        puts(env, { arg });
        return args[0];
    } else {
        ArrayObject *result = new ArrayObject { args.size() };
        Vector<Value> puts_args(args.size());
        for (size_t i = 0; i < args.size(); i++) {
            result->push(args[i]);
            puts_args.push(args[i].send(env, "inspect"_s));
        }
        puts(env, std::move(puts_args));
        return result;
    }
}

Value KernelModule::print(Env *env, Args &&args) {
    auto _stdout = env->global_get("$stdout"_s);
    assert(_stdout);
    if (args.size() == 0)
        return _stdout.send(env, "write"_s, Args { env->global_get("$_"_s) });
    // NATFIXME: Kernel.print should actually call IO.print and not
    // IO.write, but for now using IO.print causes crashes.
    // return _stdout.send(env, "print"_s, args);
    return _stdout.send(env, "write"_s, std::move(args));
}

Value KernelModule::proc(Env *env, Block *block) {
    if (block)
        return new ProcObject { block };
    else
        env->raise("ArgumentError", "tried to create Proc object without a block");
}

Value KernelModule::puts(Env *env, Args &&args) {
    auto _stdout = env->global_get("$stdout"_s);
    return _stdout.send(env, "puts"_s, std::move(args));
}

Value KernelModule::raise(Env *env, Args &&args) {
    auto exception = ExceptionObject::create_for_raise(env, std::move(args), env->exception(), true);
    env->raise_exception(exception);
}

Value KernelModule::Rational(Env *env, Value x, Value y, Value exception) {
    return Rational(env, x, y, exception ? exception.is_true() : true);
}

Value KernelModule::Rational(Env *env, Value x, Value y, bool exception) {
    if (y) {
        if (x.is_integer() && y.is_integer())
            return Rational(env, x.integer(), y.integer());

        x = Float(env, x, exception);
        if (!x)
            return nullptr;

        y = Float(env, y, exception);
        if (!y)
            return nullptr;

        if (y->as_float()->is_zero())
            env->raise("ZeroDivisionError", "divided by 0");

        return Rational(env, x->as_float()->to_double() / y->as_float()->to_double());
    } else {
        if (x.is_integer()) {
            return new RationalObject { x.integer(), Value::integer(1) };
        }

        if (!exception)
            return nullptr;

        if (x.is_nil())
            env->raise("TypeError", "can't convert {} into Rational", x.klass()->inspect_str());

        if (x.respond_to(env, "to_r"_s)) {
            auto result = x->public_send(env, "to_r"_s);
            result.assert_type(env, Object::Type::Rational, "Rational");
            return result;
        }

        x = Float(env, x, exception);
        if (!x)
            return nullptr;

        return Rational(env, x->as_float()->to_double());
    }
}

RationalObject *KernelModule::Rational(Env *env, class Integer &x, class Integer &y) {
    Value gcd = IntegerMethods::gcd(env, x, y).integer();
    class Integer numerator = x / gcd;
    class Integer denominator = y / gcd;
    return RationalObject::create(env, numerator, denominator);
}

RationalObject *KernelModule::Rational(Env *env, double arg) {
    class Integer radix(FLT_RADIX);
    auto power = Value::integer(DBL_MANT_DIG);
    auto y = IntegerMethods::pow(env, radix, power).integer();

    int exponent;
    FloatObject *significand = new FloatObject { std::frexp(arg, &exponent) };
    auto x = significand->mul(env, y)->as_float()->to_i(env).integer();

    class Integer two(2);
    class Integer exp = exponent;
    if (exp < 0)
        y = y * IntegerMethods::pow(env, two, -exp).integer();
    else
        x = x * IntegerMethods::pow(env, two, exp).integer();

    return Rational(env, x, y);
}

Value KernelModule::sleep(Env *env, Value length) {
    if (FiberObject::scheduler_is_relevant()) {
        if (!length) length = NilObject::the();
        return FiberObject::scheduler().send(env, "kernel_sleep"_s, { length });
    }

    if (!length || length.is_nil())
        return ThreadObject::current()->sleep(env, -1.0);

    float secs;
    if (length.is_integer()) {
        secs = length.integer().to_nat_int_t();
    } else if (length.is_float()) {
        secs = length->as_float()->to_double();
    } else if (length.is_rational()) {
        secs = length->as_rational()->to_f(env)->as_float()->to_double();
    } else if (length.respond_to(env, "divmod"_s)) {
        auto divmod = length.send(env, "divmod"_s, { Value::integer(1) })->as_array();
        secs = divmod->at(0).to_f(env)->as_float()->to_double();
        secs += divmod->at(1).to_f(env)->as_float()->to_double();
    } else {
        env->raise("TypeError", "can't convert {} into time interval", length.klass()->inspect_str());
    }

    if (secs < 0.0)
        env->raise("ArgumentError", "time interval must not be negative");

    return ThreadObject::current()->sleep(env, secs);
}

Value KernelModule::spawn(Env *env, Args &&args) {
    pid_t pid;
    int result;

    Vector<char *> new_env;

    Defer free_new_env([&]() {
        for (auto str : new_env) {
            free(str);
        }
    });

    if (args.size() >= 1 && (args.at(0).is_hash() || args.at(0).respond_to(env, "to_hash"_s))) {
        auto hash = args.shift().to_hash(env);
        for (auto ep = environ; *ep; ep++)
            new_env.push(strdup(*ep));
        for (auto pair : *hash) {
            auto key = pair.key.to_str(env);
            if (key->include(env, '='))
                env->raise("ArgumentError", "environment name contains a equal : {}", key->string());
            if (key->include(env, '\0'))
                env->raise("ArgumentError", "string contains null byte");
            auto val = pair.val.to_str(env);
            if (val->include(env, '\0'))
                env->raise("ArgumentError", "string contains null byte");
            auto combined = String::format(
                "{}={}",
                key->string(),
                val->string());
            new_env.push(strdup(combined.c_str()));
        }
        new_env.push(nullptr);
    }

    args.ensure_argc_at_least(env, 1);

    if (args.size() == 1) {
        auto arg = args.at(0).to_str(env);
        bool needs_escaping = false;
        for (auto c : *arg) {
            if (c == '"' || c == '\'' || c == '$' || c == '<' || c == '>') {
                needs_escaping = true;
                break;
            }
        }
        if (needs_escaping) {
            const char *cmd[] = {
                "sh",
                "-c",
                arg->c_str(),
                nullptr
            };
            result = posix_spawnp(
                &pid,
                "/bin/sh",
                NULL,
                NULL,
                const_cast<char *const *>(cmd),
                new_env.is_empty() ? environ : new_env.data());
        } else {
            auto splitter = new RegexpObject { env, "\\s+" };
            auto split = arg->split(env, splitter, 0)->as_array();
            const char *cmd[split->size() + 1];
            for (size_t i = 0; i < split->size(); i++) {
                cmd[i] = split->at(i)->as_string()->c_str();
            }
            cmd[split->size()] = nullptr;
            result = posix_spawnp(
                &pid,
                cmd[0],
                NULL,
                NULL,
                const_cast<char *const *>(cmd),
                new_env.is_empty() ? environ : new_env.data());
        }
    } else {
        const char *cmd[args.size() + 1];
        for (size_t i = 0; i < args.size(); i++) {
            cmd[i] = args[i].to_str(env)->c_str();
        }
        cmd[args.size()] = nullptr;
        auto program = args[0].to_str(env);
        result = posix_spawnp(
            &pid,
            program->c_str(),
            NULL,
            NULL,
            const_cast<char *const *>(cmd),
            new_env.is_empty() ? environ : new_env.data());
    }

    if (result != 0)
        env->raise_errno();

    return Value::integer(pid);
}

Value KernelModule::String(Env *env, Value value) {
    if (value.is_string()) {
        return value;
    }

    auto to_s = "to_s"_s;

    if (!respond_to_method(env, value, to_s, true) || !value.respond_to(env, to_s))
        env->raise("TypeError", "can't convert {} into String", value.klass()->inspect_str());

    value = value.send(env, to_s);
    value.assert_type(env, Object::Type::String, "String");
    return value;
}

Value KernelModule::test(Env *env, Value cmd, Value file) {
    switch (cmd.to_str(env)->string()[0]) {
    case 'A':
        return FileObject::stat(env, file)->as_file_stat()->atime(env);
    case 'C':
        return FileObject::stat(env, file)->as_file_stat()->ctime(env);
    case 'd':
        return bool_object(FileObject::is_directory(env, file));
    case 'e':
        return bool_object(FileObject::exist(env, file));
    case 'f':
        return bool_object(FileObject::is_file(env, file));
    case 'l':
        return bool_object(FileObject::is_symlink(env, file));
    case 'M':
        return FileObject::stat(env, file)->as_file_stat()->mtime(env);
    case 'r':
        return bool_object(FileObject::is_readable(env, file));
    case 'R':
        return bool_object(FileObject::is_readable_real(env, file));
    case 'w':
        return bool_object(FileObject::is_writable(env, file));
    case 'W':
        return bool_object(FileObject::is_writable_real(env, file));
    default:
        env->raise("ArgumentError", "unknown command '{}'", cmd.to_str(env)->string()[0]);
    }
    NAT_UNREACHABLE();
}

Value KernelModule::this_method(Env *env) {
    auto method = env->caller()->current_method();
    if (method)
        return SymbolObject::intern(method->original_name());
    return NilObject::the();
}

Value KernelModule::throw_method(Env *env, Value name, Value value) {
    if (!env->has_catch(name)) {
        auto klass = GlobalEnv::the()->Object()->const_fetch("UncaughtThrowError"_s)->as_class();
        auto message = StringObject::format("uncaught throw {}", name.inspect_str(env));
        auto exception = Object::_new(env, klass, { name, value, message }, nullptr)->as_exception();
        env->raise_exception(exception);
    }

    throw new ThrowCatchException { name, value };
}

Value KernelModule::klass_obj(Env *env, Value self) {
    if (self.is_integer())
        return GlobalEnv::the()->Integer();
    else if (self.klass())
        return self.klass();
    else
        return NilObject::the();
}

Value KernelModule::define_singleton_method(Env *env, Value self, Value name, Block *block) {
    env->ensure_block_given(block);
    SymbolObject *name_obj = name.to_symbol(env, Value::Conversion::Strict);
    Object::define_singleton_method(env, self, name_obj, block);
    return name_obj;
}

Value KernelModule::dup(Env *env, Value self) {
    // The semantics here are wrong. For each class, we need to define `initialize_copy`.
    // Doing that all at once is a pain, so we'll split off classes here:
    // classes that have their own `initialize_copy` method get the `dup_better` code path,
    // while the rest get the old, wrong code path.
    switch (self->type()) {
    case Object::Type::Array:
    case Object::Type::Hash:
    case Object::Type::String:
        return dup_better(env, self);
    default:
        return self->duplicate(env);
    }
}

Value KernelModule::dup_better(Env *env, Value self) {
    auto dup = self->allocate(env, self.klass(), {}, nullptr);
    dup->copy_instance_variables(self);
    dup.send(env, "initialize_dup"_s, { self });
    return dup;
}

Value KernelModule::extend(Env *env, Value self, Args &&args) {
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i]->type() == Object::Type::Module) {
            args[i]->as_module()->send(env, "extend_object"_s, { self });
        } else {
            env->raise("TypeError", "wrong argument type {} (expected Module)", args[i].klass()->inspect_str());
        }
    }
    return self;
}

Value KernelModule::hash(Env *env, Value self) {
    if (self.is_integer())
        return Value::integer(self.integer().to_string().djb2_hash());

    switch (self->type()) {
    // NOTE: string "foo" and symbol :foo will get the same hash.
    // That's probably ok, but maybe worth revisiting.
    case Object::Type::String:
        return Value::integer(self->as_string()->string().djb2_hash());
    case Object::Type::Symbol:
        return Value::integer(self->as_symbol()->string().djb2_hash());
    default: {
        StringObject *inspected = self.send(env, "inspect"_s)->as_string();
        nat_int_t hash_value = inspected->string().djb2_hash();
        return Value::integer(hash_value);
    }
    }
}

Value KernelModule::initialize_copy(Env *env, Value self, Value object) {
    if (object == self)
        return self;

    self.assert_not_frozen(env);
    if (self.klass() != object.klass())
        env->raise("TypeError", "initialize_copy should take same class object");

    return self;
}

Value KernelModule::inspect(Env *env, Value value) {
    if (value.is_module() && value->as_module()->name())
        return new StringObject { value->as_module()->name().value() };
    else
        return StringObject::format("#<{}:{}>", value.klass()->inspect_str(), value->pointer_id());
}

bool KernelModule::instance_variable_defined(Env *env, Value self, Value name_val) {
    if (self.is_integer())
        return false;
    switch (self->type()) {
    case Object::Type::Nil:
    case Object::Type::True:
    case Object::Type::False:
    case Object::Type::Float:
    case Object::Type::Symbol:
        return false;
    default:
        break;
    }
    auto name = Object::to_instance_variable_name(env, name_val);
    return self->ivar_defined(env, name);
}

Value KernelModule::instance_variable_get(Env *env, Value self, Value name_val) {
    if (self.is_integer() || self.is_float())
        return NilObject::the();

    auto name = Object::to_instance_variable_name(env, name_val);
    return self->ivar_get(env, name);
}

Value KernelModule::instance_variable_set(Env *env, Value self, Value name_val, Value value) {
    auto name = Object::to_instance_variable_name(env, name_val);
    self.assert_not_frozen(env);
    self->ivar_set(env, name, value);
    return value;
}

Value KernelModule::instance_variables(Env *env, Value self) {
    if (self.is_integer())
        return new ArrayObject;

    return self->instance_variables(env);
}

bool KernelModule::is_a(Env *env, Value self, Value module) {
    if (!module.is_module())
        env->raise("TypeError", "class or module required");

    if (self.is_integer())
        return GlobalEnv::the()->Integer()->ancestors_includes(env, module->as_module());

    return self.is_a(env, module->as_module());
}

Value KernelModule::loop(Env *env, Value self, Block *block) {
    if (!block) {
        auto infinity_fn = [](Env *env, Value, Args &&, Block *) -> Value {
            return FloatObject::positive_infinity(env);
        };
        auto size_block = new Block { *env, self, infinity_fn, 0 };
        return self.send(env, "enum_for"_s, { "loop"_s }, size_block);
    }

    try {
        for (;;) {
            block->run(env, {}, nullptr);
        }
        return NilObject::the();
    } catch (ExceptionObject *exception) {
        auto StopIteration = find_top_level_const(env, "StopIteration"_s);
        if (Value(exception).is_a(env, StopIteration)) {
            GlobalEnv::the()->set_rescued(true);
            return exception->send(env, "result"_s);
        } else {
            throw exception;
        }
    }
}

Value KernelModule::method(Env *env, Value self, Value name) {
    auto name_symbol = name.to_symbol(env, Value::Conversion::Strict);
    auto singleton = self.singleton_class();
    auto module = singleton ? singleton : self.klass();
    auto method_info = module->find_method(env, name_symbol);
    if (!method_info.is_defined()) {
        auto respond_to_missing = module->find_method(env, "respond_to_missing?"_s);
        if (respond_to_missing.is_defined()) {
            if (respond_to_missing.method()->call(env, self, { name_symbol, TrueObject::the() }, nullptr).is_truthy()) {
                auto method_missing = module->find_method(env, "method_missing"_s);
                if (method_missing.is_defined()) {
                    return new MethodObject { self, method_missing.method(), name_symbol };
                }
            }
        }
        env->raise("NoMethodError", "undefined method `{}' for {}:Class", name_symbol->inspect_str(env), self.klass()->inspect_str());
    }
    return new MethodObject { self, method_info.method() };
}

Value KernelModule::methods(Env *env, Value self, Value regular_val) {
    bool regular = regular_val ? regular_val.is_truthy() : true;
    if (regular) {
        if (self->singleton_class()) {
            return self->singleton_class()->instance_methods(env, TrueObject::the());
        } else {
            return self.klass()->instance_methods(env, TrueObject::the());
        }
    }
    if (self->singleton_class())
        return self->singleton_class()->instance_methods(env, FalseObject::the());
    else
        return new ArrayObject {};
}

bool KernelModule::neqtilde(Env *env, Value self, Value other) {
    return self.send(env, "=~"_s, { other }).is_falsey();
}

Value KernelModule::private_methods(Env *env, Value self, Value recur) {
    if (self->singleton_class())
        return self->singleton_class()->private_instance_methods(env, TrueObject::the());
    else
        return self.klass()->private_instance_methods(env, recur);
}

Value KernelModule::protected_methods(Env *env, Value self, Value recur) {
    if (self->singleton_class())
        return self->singleton_class()->protected_instance_methods(env, TrueObject::the());
    else
        return self.klass()->protected_instance_methods(env, recur);
}

Value KernelModule::public_methods(Env *env, Value self, Value recur) {
    if (self.singleton_class())
        return self.singleton_class()->public_instance_methods(env, TrueObject::the());
    else
        return self.klass()->public_instance_methods(env, recur);
}

Value KernelModule::remove_instance_variable(Env *env, Value self, Value name_val) {
    auto name = Object::to_instance_variable_name(env, name_val);
    self->assert_not_frozen(env);
    return self->ivar_remove(env, name);
}

bool KernelModule::respond_to_method(Env *env, Value self, Value name_val, Value include_all_val) {
    bool include_all = include_all_val ? include_all_val.is_truthy() : false;
    return respond_to_method(env, self, name_val, include_all);
}

bool KernelModule::respond_to_method(Env *env, Value self, Value name_val, bool include_all) {
    auto name_symbol = name_val.to_symbol(env, Value::Conversion::Strict);

    ClassObject *klass = self.singleton_class();
    if (!klass)
        klass = self.klass();

    auto method_info = klass->find_method(env, name_symbol);
    if (!method_info.is_defined()) {
        if (klass->find_method(env, "respond_to_missing?"_s).is_defined()) {
            return self.send(env, "respond_to_missing?"_s, { name_val, bool_object(include_all) }).is_truthy();
        }
        return false;
    }

    if (include_all)
        return true;

    MethodVisibility visibility = method_info.visibility();
    if (visibility == MethodVisibility::Public) {
        return true;
    } else if (klass->find_method(env, "respond_to_missing?"_s).is_defined()) {
        return self.send(env, "respond_to_missing?"_s, { name_val, bool_object(include_all) }).is_truthy();
    } else {
        return false;
    }
}

Value KernelModule::tap(Env *env, Value self, Block *block) {
    Value args[] = { self };
    if (!block)
        env->raise("LocalJumpError", "no block given (yield)");
    block->run(env, Args(1, args), nullptr);
    return self;
}

}
