#include "natalie.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/thread_object.hpp"
#include "natalie/throw_catch_exception.hpp"

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

static bool exception_argument_to_bool(Env *env, Optional<Value> exception_arg) {
    if (!exception_arg)
        return true;
    auto exception = exception_arg.value();
    if (!exception.is_false() && !exception.is_true())
        env->raise("ArgumentError", "expected true or false as exception: {}", exception.inspected(env));
    return exception.is_true();
}

Value KernelModule::Array(Env *env, Value value) {
    return Natalie::to_ary(env, value, true);
}

Value KernelModule::abort_method(Env *env, Optional<Value> message_arg) {
    auto SystemExit = GlobalEnv::the()->Object()->const_fetch("SystemExit"_s);
    ExceptionObject *exception;

    if (message_arg) {
        auto message = message_arg.value();
        if (!message.is_string())
            message = message.to_str(env);
        message.assert_type(env, Object::Type::String, "String");

        exception = SystemExit.send(env, "new"_s, { Value::integer(1), message }).as_exception();

        auto out = env->global_get("$stderr"_s);
        out.send(env, "puts"_s, { message });
    } else {
        exception = SystemExit.send(env, "new"_s, { Value::integer(1) }).as_exception();
    }

    env->raise_exception(exception);

    return Value::nil();
}

Value KernelModule::at_exit(Env *env, Block *block) {
    ArrayObject *at_exit_handlers = env->global_get("$NAT_at_exit_handlers"_s).as_array();
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
        out = StringObject::create(buf);
        while (1) {
            result = fgets(buf, NAT_SHELL_READ_BYTES, process);
            if (!result) break;
            out->append(buf);
        }
    } else {
        out = StringObject::create();
    }
    int status = pclose2(process, pid);
    set_status_object(env, pid, status);
    return out;
}

Value KernelModule::binding(Env *env) {
    return new BindingObject { env };
}

Value KernelModule::caller(Env *env, Optional<Value> start_arg, Optional<Value> length) {
    auto start = start_arg.value_or(Value::integer(1)); // 1 = remove the frame for the call site of Kernel#caller
    auto backtrace = env->backtrace();
    auto ary = backtrace->to_ruby_array();
    ary->shift(); // remove the frame for Kernel#caller itself
    if (start.is_range()) {
        ary = ary->ref(env, start).as_array();
    } else {
        ary->shift(env, start);
        if (length)
            ary = ary->first(env, length).as_array();
    }
    return ary;
}

Value KernelModule::caller_locations(Env *env, Optional<Value> start_arg, Optional<Value> length) {
    auto start = start_arg.value_or(Value::integer(1)); // 1 = remove the frame for the call site of Kernel#caller_locations
    auto backtrace = env->backtrace();
    auto ary = backtrace->to_ruby_backtrace_locations_array();
    ary->shift(); // remove the frame for Kernel#caller_locations itself
    if (start.is_range()) {
        ary = ary->ref(env, start).as_array();
    } else {
        ary->shift(env, start);
        if (length)
            ary = ary->first(env, length).as_array();
    }
    return ary;
}

Value KernelModule::catch_method(Env *env, Optional<Value> name_arg, Block *block) {
    if (!block)
        env->raise("LocalJumpError", "no block given");
    auto name = name_arg.value_or([]() { return new Object; });

    try {
        Env block_env { env };
        block_env.set_catch(name);
        return block->run(&block_env, { name }, nullptr);
    } catch (ThrowCatchException *e) {
        if (Object::equal(e->get_name(), name))
            return e->get_value().value_or(Value::nil());
        else
            throw e;
    }
}

Value KernelModule::Complex(Env *env, Value real, Optional<Value> imaginary, Optional<Value> exception) {
    return Complex(env, real, imaginary, exception_argument_to_bool(env, exception));
}

Value KernelModule::Complex(Env *env, Value real, Optional<Value> imaginary, bool exception) {
    if (real.is_string())
        return Complex(env, real.as_string(), exception, false);

    if (real.is_complex() && !imaginary)
        return real;

    if (real.is_complex() && imaginary.value().is_complex())
        return real.send(env, "+"_s, { imaginary.value().send(env, "*"_s, { new ComplexObject { Value::integer(0), Value::integer(1) } }) });

    auto is_numeric = [&env](Value val) -> bool {
        if (val.is_numeric() || val.is_rational() || val.is_complex())
            return true;
        if (!val.respond_to(env, "real?"_s))
            return false;
        auto Numeric = GlobalEnv::the()->Object()->const_get(env, "Numeric"_s);
        return is_a(env, val, Numeric);
    };

    if (is_numeric(real)) {
        if (!imaginary) {
            return new ComplexObject { real };
        } else if (is_numeric(imaginary.value())) {
            return new ComplexObject { real, imaginary.value() };
        }
    }

    if (exception)
        env->raise("TypeError", "can't convert {} into Complex", real.klass()->inspect_module());

    return Value::nil();
}

Value KernelModule::Complex(Env *env, StringObject *input, bool exception, bool string_to_c) {
    auto error = [&]() -> Value {
        if (exception)
            env->raise("ArgumentError", "invalid value for convert(): \"{}\"", input->string());
        return Value::nil();
    };
    if (!input->is_ascii_only()) {
        if (string_to_c)
            return new ComplexObject { Value::integer(0) };
        return error();
    }
    enum class State {
        Start,
        Real,
        Imag,
        Finished,
    };
    enum class Type {
        Undefined,
        Integer,
        Float,
        Scientific,
        Rational,
    };
    auto state = State::Start;
    auto real_type = Type::Undefined;
    auto imag_type = Type::Undefined;
    bool polar = false;
    const char *real_start = nullptr;
    const char *real_end = nullptr;
    const char *imag_start = nullptr;
    const char *imag_end = nullptr;
    auto *curr_type = &real_type;
    const char **curr_start = &real_start;
    const char **curr_end = &real_end;
    auto new_real = Value::integer(0);
    auto new_imag = Value::integer(0);
    for (const char *c = input->c_str(); c < input->c_str() + input->bytesize(); c++) {
        if (*c == 0) {
            if (string_to_c) {
                state = State::Finished;
                continue;
            } else {
                if (exception)
                    env->raise("ArgumentError", "string contains null byte");
                return Value::nil();
            }
        } else if (*c <= 0x08 || (*c >= 0x0e && *c <= 0x1f) || *c >= 0x7f) {
            if (string_to_c) {
                state = State::Finished;
                continue;
            } else {
                return error();
            }
        }
        switch (state) {
        case State::Start:
            if ((*c >= '0' && *c <= '9') || *c == '+' || *c == '-') {
                *curr_start = *curr_end = c;
                state = State::Real;
                *curr_type = Type::Integer;
            } else if (*c == 'i' || *c == 'I' || *c == 'j' || *c == 'J') {
                new_imag = Value::integer(1);
                state = State::Finished;
            } else if (!string_to_c && *c != ' ' && *c != '\t' && *c != '\n' && *c != '\v' && *c != '\f' && *c != '\r') {
                return error();
            }
            break;
        case State::Real:
        case State::Imag:
            if (*c >= '0' && *c <= '9') {
                *curr_end = c;
            } else if (*c == '_') {
                if (c[1] && c[1] >= '0' && c[1] <= '9') {
                    continue; // Skip single underscore, only if it is part of a number
                } else if (string_to_c) {
                    imag_start = imag_end = nullptr;
                    state = State::Finished;
                } else {
                    return Value::nil();
                }
            } else if (*c == '.') {
                if (*curr_type == Type::Integer) {
                    *curr_type = Type::Float;
                    *curr_end = c;
                } else if (*curr_type == Type::Float) {
                    return error();
                } else {
                    return error();
                }
            } else if (*c == '/') {
                if (*curr_type == Type::Rational)
                    return error();
                *curr_type = Type::Rational;
                *curr_end = c;
            } else if (*c == 'e' || *c == 'E') {
                if (*curr_type == Type::Scientific)
                    return error();
                else if (*curr_type != Type::Rational)
                    *curr_type = Type::Scientific;
                *curr_end = c;
            } else if (*c == '@') {
                if (polar)
                    return error();
                if (state != State::Real)
                    return error();
                if (!c[1])
                    return error();
                polar = true;
                curr_start = &imag_start;
                curr_end = &imag_end;
                *curr_start = *curr_end = c;
                curr_type = &imag_type;
                *curr_type = Type::Integer;
                state = State::Imag;
            } else if (*c == '+' || *c == '-') {
                if (*curr_start && *curr_start == *curr_end && (**curr_end == '-' || **curr_end == '+'))
                    return error();
                if (state == State::Imag) {
                    if (!polar && (!*curr_start || *curr_start != *curr_end))
                        return error();
                } else {
                    curr_start = &imag_start;
                    curr_end = &imag_end;
                    *curr_start = *curr_end = c;
                    curr_type = &imag_type;
                    *curr_type = Type::Integer;
                    state = State::Imag;
                }
            } else if (*c == 'i' || *c == 'I' || *c == 'j' || *c == 'J') {
                if (*curr_start && *curr_start == *curr_end && (**curr_end == '-' || **curr_end == '+')) {
                    // Corner case: '-i' or '+i'
                    new_imag = Value::integer(**curr_end == '-' ? -1 : 1);
                    *curr_start = nullptr;
                    *curr_end = nullptr;
                }
                if (state == State::Real) {
                    imag_type = real_type;
                    imag_start = real_start;
                    imag_end = c - 1;
                    real_type = Type::Undefined;
                    real_start = nullptr;
                    real_end = nullptr;
                }
                state = State::Finished;
            } else {
                return error();
            }
            break;
        case State::Finished:
            if (!string_to_c && *c != ' ' && *c != '\t' && *c != '\n' && *c != '\v' && *c != '\f' && *c != '\r')
                return error();
        }
    }

    switch (state) {
    case State::Start:
        if (string_to_c)
            return new ComplexObject { Value::integer(0), Value::integer(0) };
        return error();
    default: {
        if (real_start != nullptr && real_end != nullptr) {
            auto tmp = StringObject::create(real_start, static_cast<size_t>(real_end - real_start + 1));
            switch (real_type) {
            case Type::Integer:
                new_real = Integer(env, tmp);
                break;
            case Type::Float:
            case Type::Scientific:
                new_real = Float(env, tmp);
                break;
            case Type::Rational:
                new_real = Rational(env, tmp);
                break;
            case Type::Undefined:
                return error();
            }
        }
        if (imag_start != nullptr && imag_end != nullptr) {
            if (polar)
                imag_start++; // skip '@'
            auto tmp = StringObject::create(imag_start, static_cast<size_t>(imag_end - imag_start + 1));
            switch (imag_type) {
            case Type::Integer:
                new_imag = Integer(env, tmp);
                break;
            case Type::Float:
            case Type::Scientific:
                new_imag = Float(env, tmp);
                break;
            case Type::Rational:
                new_imag = Rational(env, tmp);
                break;
            case Type::Undefined:
                return error();
            }
        }
        if (polar) {
            auto Complex = GlobalEnv::the()->Object()->const_get(env, "Complex"_s);
            return Complex.send(env, "polar"_s, { new_real, new_imag });
        }
        return new ComplexObject { new_real, new_imag };
    }
    }
}

Value KernelModule::cur_callee(Env *env) {
    auto method = env->caller()->current_method();
    if (method)
        return SymbolObject::intern(method->name());

    return Value::nil();
}

Value KernelModule::cur_dir(Env *env) {
    if (env->file() == nullptr) {
        env->raise("RuntimeError", "could not get current directory");
    } else if (strcmp(env->file(), "-e") == 0) {
        return StringObject::create(".");
    } else {
        Value relative = StringObject::create(env->file());
        StringObject *absolute = FileObject::expand_path(env, relative).as_string();
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

Value KernelModule::exit(Env *env, Optional<Value> status_arg) {
    auto status = Value::integer(0);
    if (status_arg) {
        if (status_arg.value().is_false()) {
            status = Value::integer(1);
        } else if (status_arg.value().is_integer()) {
            status = status_arg.value();
        }
    }

    ExceptionObject *exception = new ExceptionObject { find_top_level_const(env, "SystemExit"_s).as_class(), StringObject::create("exit") };
    exception->ivar_set(env, "@status"_s, status.to_int(env));
    env->raise_exception(exception);
    return Value::nil();
}

Value KernelModule::exit_bang(Env *env, Optional<Value> status) {
    env->global_get("$NAT_at_exit_handlers"_s).as_array_or_raise(env)->clear(env);
    return exit(env, status);
}

Value KernelModule::Integer(Env *env, Value value, Optional<Value> base, Optional<Value> exception) {
    nat_int_t base_int = 0; // default to zero if unset
    if (base)
        base_int = base.value().to_int(env).to_nat_int_t();
    return Integer(env, value, base_int, exception_argument_to_bool(env, exception));
}

Value KernelModule::Integer(Env *env, Value value, nat_int_t base, bool exception) {
    if (value.is_string()) {
        auto result = value.as_string()->convert_integer(env, base);
        if (result.is_nil() && exception) {
            env->raise("ArgumentError", "invalid value for Integer(): {}", value.inspected(env));
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
        auto float_obj = value.as_float();
        if (float_obj->is_nan() || float_obj->is_infinity()) {
            if (exception)
                env->raise("FloatDomainError", "{}", float_obj->to_s().as_string()->string());
            else
                return Value::nil();
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
        env->raise("TypeError", "can't convert {} into Integer", value.klass()->inspect_module());
    else
        return Value::nil();
}

Value KernelModule::Float(Env *env, Value value, Optional<Value> exception_kwarg) {
    return Float(env, value, exception_argument_to_bool(env, exception_kwarg));
}

Value KernelModule::Float(Env *env, Value value, bool exception) {
    if (value.is_float()) {
        return value;
    } else if (value.is_string()) {
        auto result = value.as_string()->convert_float();
        if (result.is_nil() && exception) {
            env->raise("ArgumentError", "invalid value for Float(): {}", value.inspected(env));
        }
        return result;
    } else if (!value.is_nil() && value.respond_to(env, "to_f"_s)) {
        auto result = value.send(env, "to_f"_s);
        if (result.is_float()) {
            return result;
        }
    }
    if (exception)
        env->raise("TypeError", "can't convert {} into Float", value.klass()->inspect_module());

    return Value::nil();
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
            return Value::nil();
        } else {
            // parent
            return Value::integer(pid);
        }
    }
}

Value KernelModule::gets(Env *env) {
    char buf[2048];
    if (!fgets(buf, 2048, stdin))
        return Value::nil();
    return StringObject::create(buf);
}

Value KernelModule::get_usage(Env *env) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        return Value::nil();

    HashObject *hash = new HashObject {};
    hash->put(env, StringObject::create("maxrss"), Value::integer(usage.ru_maxrss));
    hash->put(env, StringObject::create("ixrss"), Value::integer(usage.ru_ixrss));
    hash->put(env, StringObject::create("idrss"), Value::integer(usage.ru_idrss));
    hash->put(env, StringObject::create("isrss"), Value::integer(usage.ru_isrss));
    hash->put(env, StringObject::create("minflt"), Value::integer(usage.ru_minflt));
    hash->put(env, StringObject::create("majflt"), Value::integer(usage.ru_majflt));
    hash->put(env, StringObject::create("nswap"), Value::integer(usage.ru_nswap));
    hash->put(env, StringObject::create("inblock"), Value::integer(usage.ru_inblock));
    hash->put(env, StringObject::create("oublock"), Value::integer(usage.ru_oublock));
    hash->put(env, StringObject::create("msgsnd"), Value::integer(usage.ru_msgsnd));
    hash->put(env, StringObject::create("msgrcv"), Value::integer(usage.ru_msgrcv));
    hash->put(env, StringObject::create("nsignals"), Value::integer(usage.ru_nsignals));
    hash->put(env, StringObject::create("nvcsw"), Value::integer(usage.ru_nvcsw));
    hash->put(env, StringObject::create("nivcsw"), Value::integer(usage.ru_nivcsw));
    return hash;
}

Value KernelModule::global_variables(Env *env) {
    return GlobalEnv::the()->global_list(env);
}

Value KernelModule::Hash(Env *env, Value value) {
    if (value.is_hash())
        return value;

    if (value.is_nil() || (value.is_array() && value.as_array()->is_empty()))
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
        return Value::nil();
    } else if (args.size() == 1) {
        Value arg = args[0].send(env, "inspect"_s);
        puts(env, { arg });
        return args[0];
    } else {
        ArrayObject *result = ArrayObject::create(args.size());
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

Value KernelModule::Rational(Env *env, Value x, Optional<Value> y, Optional<Value> exception) {
    return Rational(env, x, y, exception ? exception.value().is_true() : true);
}

Value KernelModule::Rational(Env *env, Value x, Optional<Value> y_arg, bool exception) {
    if (y_arg) {
        auto y = y_arg.value();
        if (x.is_integer() && y.is_integer())
            return Rational(env, x.integer(), y.integer());

        x = Float(env, x, exception);
        if (x.is_nil())
            return Value::nil();

        y = Float(env, y, exception);
        if (y.is_nil())
            return Value::nil();

        if (y.as_float()->is_zero())
            env->raise("ZeroDivisionError", "divided by 0");

        return Rational(env, x.as_float()->to_double() / y.as_float()->to_double());
    } else {
        if (x.is_integer()) {
            return new RationalObject { x.integer(), Value::integer(1) };
        }

        if (!exception)
            return Value::nil();

        if (x.is_nil())
            env->raise("TypeError", "can't convert {} into Rational", x.klass()->inspect_module());

        if (x.respond_to(env, "to_r"_s)) {
            auto result = x->public_send(env, "to_r"_s);
            result.assert_type(env, Object::Type::Rational, "Rational");
            return result;
        }

        x = Float(env, x, exception);
        if (x.is_nil())
            return Value::nil();

        return Rational(env, x.as_float()->to_double());
    }
}

RationalObject *KernelModule::Rational(Env *env, class Integer x, class Integer y) {
    auto gcd = IntegerMethods::gcd(env, x, y).integer();
    class Integer numerator = x / gcd;
    class Integer denominator = y / gcd;
    return RationalObject::create(env, numerator, denominator);
}

RationalObject *KernelModule::Rational(Env *env, double arg) {
    class Integer radix(FLT_RADIX);
    auto power = Value::integer(DBL_MANT_DIG);
    auto y = IntegerMethods::pow(env, radix, power).integer();

    int exponent;
    FloatObject *significand = FloatObject::create(std::frexp(arg, &exponent));
    auto x = significand->mul(env, y).as_float()->to_i(env).integer();

    class Integer two(2);
    class Integer exp = exponent;
    if (exp < 0)
        y = y * IntegerMethods::pow(env, two, -exp).integer();
    else
        x = x * IntegerMethods::pow(env, two, exp).integer();

    return Rational(env, x, y);
}

Value KernelModule::sleep(Env *env, Optional<Value> length_arg) {
    if (FiberObject::scheduler_is_relevant()) {
        auto length = length_arg.value_or(Value::nil());
        return FiberObject::scheduler().send(env, "kernel_sleep"_s, { length });
    }

    if (!length_arg || length_arg.value().is_nil())
        return ThreadObject::current()->sleep(env, -1.0);

    auto length = length_arg.value();

    float secs;
    if (length.is_integer()) {
        secs = length.integer().to_nat_int_t();
    } else if (length.is_float()) {
        secs = length.as_float()->to_double();
    } else if (length.is_rational()) {
        secs = length.as_rational()->to_f(env).as_float()->to_double();
    } else if (length.respond_to(env, "divmod"_s)) {
        auto divmod = length.send(env, "divmod"_s, { Value::integer(1) }).as_array();
        secs = divmod->at(0).to_f(env)->to_double();
        secs += divmod->at(1).to_f(env)->to_double();
    } else {
        env->raise("TypeError", "can't convert {} into time interval", length.klass()->inspect_module());
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
            auto split = arg->split(env, splitter, 0).as_array();
            const char *cmd[split->size() + 1];
            for (size_t i = 0; i < split->size(); i++) {
                cmd[i] = split->at(i).as_string()->c_str();
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

    if (!respond_to_method(env, value, Value(to_s), true) || !value.respond_to(env, to_s))
        env->raise("TypeError", "can't convert {} into String", value.klass()->inspect_module());

    value = value.send(env, to_s);
    value.assert_type(env, Object::Type::String, "String");
    return value;
}

Value KernelModule::test(Env *env, Value cmd, Value file) {
    switch (cmd.to_str(env)->string()[0]) {
    case 'A':
        return FileObject::stat(env, file).as_file_stat()->atime(env);
    case 'C':
        return FileObject::stat(env, file).as_file_stat()->ctime(env);
    case 'd':
        return bool_object(FileObject::is_directory(env, file));
    case 'e':
        return bool_object(FileObject::exist(env, file));
    case 'f':
        return bool_object(FileObject::is_file(env, file));
    case 'l':
        return bool_object(FileObject::is_symlink(env, file));
    case 'M':
        return FileObject::stat(env, file).as_file_stat()->mtime(env);
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
    return Value::nil();
}

Value KernelModule::throw_method(Env *env, Value name, Optional<Value> value) {
    if (!env->has_catch(name)) {
        auto klass = GlobalEnv::the()->Object()->const_fetch("UncaughtThrowError"_s).as_class();
        auto message = StringObject::format("uncaught throw {}", name.inspected(env));
        auto exception = Object::_new(env, klass, { name, value.value_or(Value::nil()), message }, nullptr).as_exception();
        env->raise_exception(exception);
    }

    throw new ThrowCatchException { name, value.value_or(Value::nil()) };
}

Value KernelModule::klass_obj(Env *env, Value self) {
    if (self.klass())
        return self.klass();
    else
        return Value::nil();
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
    switch (self.type()) {
    case Object::Type::Nil:
    case Object::Type::Integer:
    case Object::Type::True:
    case Object::Type::False:
        return self;
    case Object::Type::Array:
    case Object::Type::Hash:
    case Object::Type::String:
        return dup_better(env, self);
    default: {
        auto result = self->duplicate(env);
        result.send(env, "initialize_copy"_s, { self });
        return result;
    }
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
        if (args[i].type() == Object::Type::Module) {
            args[i].as_module()->send(env, "extend_object"_s, { self });
        } else {
            env->raise("TypeError", "wrong argument type {} (expected Module)", args[i].klass()->inspect_module());
        }
    }
    return self;
}

Value KernelModule::hash(Env *env, Value self) {
    switch (self.type()) {
    case Object::Type::Integer:
        return Value::integer(HashKeyHandler<TM::String>::hash(self.integer().to_string()));
    // NOTE: string "foo" and symbol :foo will get the same hash.
    // That's probably ok, but maybe worth revisiting.
    case Object::Type::String:
        return Value::integer(HashKeyHandler<TM::String>::hash(self.as_string()->string()));
    case Object::Type::Symbol:
        return Value::integer(HashKeyHandler<TM::String>::hash(self.as_symbol()->string()));
    default: {
        StringObject *inspected = self.send(env, "inspect"_s).as_string();
        nat_int_t hash_value = HashKeyHandler<TM::String>::hash(inspected->string());
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
    if (value.is_module() && value.as_module()->name())
        return StringObject::create(value.as_module()->name().value());
    else
        return StringObject::format("#<{}:{}>", value.klass()->inspect_module(), value->pointer_id());
}

bool KernelModule::instance_variable_defined(Env *env, Value self, Value name_val) {
    if (!self.has_instance_variables())
        return false;
    auto name = Object::to_instance_variable_name(env, name_val);
    return self->ivar_defined(env, name);
}

Value KernelModule::instance_variable_get(Env *env, Value self, Value name_val) {
    auto name = Object::to_instance_variable_name(env, name_val);
    if (!self.has_instance_variables())
        return Value::nil();
    return self->ivar_get(env, name);
}

Value KernelModule::instance_variable_set(Env *env, Value self, Value name_val, Value value) {
    auto name = Object::to_instance_variable_name(env, name_val);
    self.assert_not_frozen(env);
    self->ivar_set(env, name, value);
    return value;
}

Value KernelModule::instance_variables(Env *env, Value self) {
    if (!self.has_instance_variables())
        return ArrayObject::create();

    return self->instance_variables(env);
}

bool KernelModule::is_a(Env *env, Value self, Value module) {
    if (!module.is_module())
        env->raise("TypeError", "class or module required");

    return self.is_a(env, module.as_module());
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
        return Value::nil();
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
            if (respond_to_missing.method()->call(env, self, { name_symbol, Value::True() }, nullptr).is_truthy()) {
                auto method_missing = module->find_method(env, "method_missing"_s);
                if (method_missing.is_defined()) {
                    return new MethodObject { self, method_missing.method(), name_symbol };
                }
            }
        }
        env->raise("NoMethodError", "undefined method `{}' for {}:Class", name_symbol->inspected(env), self.klass()->inspect_module());
    }
    return new MethodObject { self, method_info.method() };
}

Value KernelModule::methods(Env *env, Value self, Optional<Value> regular_val) {
    bool regular = regular_val ? regular_val.value().is_truthy() : true;
    if (regular) {
        if (self->singleton_class()) {
            return self->singleton_class()->instance_methods(env, Value::True());
        } else {
            return self.klass()->instance_methods(env, Value::True());
        }
    }
    if (self->singleton_class())
        return self->singleton_class()->instance_methods(env, Value::False());
    else
        return ArrayObject::create();
}

bool KernelModule::neqtilde(Env *env, Value self, Value other) {
    return self.send(env, "=~"_s, { other }).is_falsey();
}

Value KernelModule::private_methods(Env *env, Value self, Optional<Value> recur) {
    if (self->singleton_class())
        return self->singleton_class()->private_instance_methods(env, Value::True());
    else
        return self.klass()->private_instance_methods(env, recur);
}

Value KernelModule::protected_methods(Env *env, Value self, Optional<Value> recur) {
    if (self->singleton_class())
        return self->singleton_class()->protected_instance_methods(env, Value::True());
    else
        return self.klass()->protected_instance_methods(env, recur);
}

Value KernelModule::public_methods(Env *env, Value self, Optional<Value> recur) {
    if (self.singleton_class())
        return self.singleton_class()->public_instance_methods(env, Value::True());
    else
        return self.klass()->public_instance_methods(env, recur);
}

Value KernelModule::public_send(Env *env, Value self, Args &&args, Block *block) {
    auto name = args.shift().to_symbol(env, Value::Conversion::Strict);
    return self.public_send(env->caller(), name, std::move(args), block);
}

Value KernelModule::remove_instance_variable(Env *env, Value self, Value name_val) {
    auto name = Object::to_instance_variable_name(env, name_val);
    self.assert_not_frozen(env);
    return self->ivar_remove(env, name);
}

bool KernelModule::respond_to_method(Env *env, Value self, Value name_val, Optional<Value> include_all_val) {
    bool include_all = include_all_val ? include_all_val.value().is_truthy() : false;
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

Value KernelModule::send(Env *env, Value self, Args &&args, Block *block) {
    auto name = args.shift().to_symbol(env, Value::Conversion::Strict);
    return self.send(env->caller(), name, std::move(args), block);
}

Value KernelModule::tap(Env *env, Value self, Block *block) {
    Value args[] = { self };
    if (!block)
        env->raise("LocalJumpError", "no block given (yield)");
    block->run(env, Args(1, args), nullptr);
    return self;
}

}
