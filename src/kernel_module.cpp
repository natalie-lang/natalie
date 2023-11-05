#include "natalie.hpp"

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
        if (!message->is_string())
            message = message->to_str(env);

        message->assert_type(env, Type::String, "String");

        exception = SystemExit.send(env, "new"_s, { message, Value::integer(1) })->as_exception();

        auto out = env->global_get("$stderr"_s);
        out->send(env, "write"_s, { message });
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

Value KernelModule::binding(Env *env) {
    return new BindingObject { env };
}

Value KernelModule::caller(Env *env) {
    auto backtrace = env->backtrace();
    auto ary = backtrace->to_ruby_array();
    ary->shift(); // remove the frame for Kernel#caller itself
    ary->shift(); // remove the frame for the call site of Kernel#caller
    return ary;
}

Value KernelModule::Complex(Env *env, Value real, Value imaginary, Value exception) {
    return Complex(env, real, imaginary, exception ? exception->is_true() : true);
}

Value KernelModule::Complex(Env *env, Value real, Value imaginary, bool exception) {

    if (imaginary == nullptr) {
        return new ComplexObject { real };
    } else if (real->is_string()) {
        // NATFIXME: Add support for strings too.
    } else {
        return new ComplexObject { real, imaginary };
    }

    if (exception)
        env->raise("TypeError", "can't convert {} into Complex", real->klass()->inspect_str());
    else
        return nullptr;
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
        if (found) {
            absolute->truncate(last_slash);
        }
        return absolute;
    }
}

Value KernelModule::define_singleton_method(Env *env, Value name, Block *block) {
    env->ensure_block_given(block);
    SymbolObject *name_obj = name->to_symbol(env, Object::Conversion::Strict);
    Object::define_singleton_method(env, name_obj, block);
    return name_obj;
}

Value KernelModule::exit(Env *env, Value status) {
    if (!status || status->is_true()) {
        status = Value::integer(0);
    } else if (status->is_false()) {
        status = Value::integer(1);
    } else if (status->is_integer()) {
        // use status passed in
    }

    ExceptionObject *exception = new ExceptionObject { find_top_level_const(env, "SystemExit"_s)->as_class(), new StringObject { "exit" } };
    exception->ivar_set(env, "@status"_s, status->to_int(env));
    env->raise_exception(exception);
    return NilObject::the();
}

Value KernelModule::exit_bang(Env *env, Value status) {
    env->global_get("$NAT_at_exit_handlers"_s)->as_array_or_raise(env)->clear(env);
    return exit(env, status);
}

Value KernelModule::Integer(Env *env, Value value, Value base, Value exception) {
    nat_int_t base_int = 0; // default to zero if unset
    if (base) {
        if (!base->is_integer() && base->respond_to(env, "to_int"_s))
            base = base->send(env, "to_int"_s);

        // NATFIXME: Discard base argument if it still isn't an int
        // Likely a bug in Ruby: https://bugs.ruby-lang.org/issues/19349
        if (base->is_integer())
            base_int = base->as_integer()->to_nat_int_t();
    }
    return Integer(env, value, base_int, exception ? exception->is_true() : true);
}

Value KernelModule::Integer(Env *env, Value value, nat_int_t base, bool exception) {
    if (value->is_string()) {
        auto result = value->as_string()->convert_integer(env, base);
        if (!result && exception) {
            env->raise("ArgumentError", "invalid value for Integer(): {}", value->inspect_str(env));
        }
        return result;
    }
    // base can only be given for string values
    if (base)
        env->raise("ArgumentError", "Cannot give base for non-string value");

    // return Integer as-is
    if (value->is_integer())
        return Value(value);

    // Infinity/NaN cannot be converted to Integer
    if (value->is_float()) {
        auto float_obj = value->as_float();
        if (float_obj->is_nan() || float_obj->is_infinity()) {
            if (exception)
                env->raise("FloatDomainError", "{}", float_obj->to_s());
            else
                return Value(NilObject::the());
        }
    }

    if (!value->is_nil()) {
        // Try using to_int to coerce to an Integer
        if (value->respond_to(env, "to_int"_s)) {
            auto result = value.send(env, "to_int"_s);
            if (result->is_integer()) return result;
        }
        // If to_int doesn't exist or doesn't return an Integer, try to_i instead.
        if (value->respond_to(env, "to_i"_s)) {
            auto result = value.send(env, "to_i"_s);
            if (result->is_integer()) return result;
        }
    }
    if (exception)
        env->raise("TypeError", "can't convert {} into Integer", value->klass()->inspect_str());
    else
        return Value(NilObject::the());
}

Value KernelModule::Float(Env *env, Value value, Value exception) {
    return Float(env, value, exception ? exception->is_true() : true);
}

Value KernelModule::Float(Env *env, Value value, bool exception) {
    if (value->is_float()) {
        return value;
    } else if (value->is_string()) {
        auto result = value->as_string()->convert_float();
        if (!result && exception) {
            env->raise("ArgumentError", "invalid value for Float(): {}", value->inspect_str(env));
        }
        return result;
    } else if (!value->is_nil() && value->respond_to(env, "to_f"_s)) {
        auto result = value.send(env, "to_f"_s);
        if (result->is_float()) {
            return result;
        }
    }
    if (exception)
        env->raise("TypeError", "can't convert {} into Float", value->klass()->inspect_str());
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
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { Value::integer(pid) }, nullptr);
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
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return NilObject::the();
    }
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

Value KernelModule::Hash(Env *env, Value value) {
    if (value->is_hash()) {
        return value;
    }

    if (value->is_nil() || (value->is_array() && value->as_array()->is_empty())) {
        return new HashObject;
    }

    return value->to_hash(env);
}

Value KernelModule::hash(Env *env) {
    switch (type()) {
    // NOTE: string "foo" and symbol :foo will get the same hash.
    // That's probably ok, but maybe worth revisiting.
    case Type::String:
        return Value::integer(as_string()->string().djb2_hash());
    case Type::Symbol:
        return Value::integer(as_symbol()->string().djb2_hash());
    default: {
        StringObject *inspected = send(env, "inspect"_s)->as_string();
        nat_int_t hash_value = inspected->string().djb2_hash();
        return Value::integer(hash_value);
    }
    }
}

Value KernelModule::initialize_copy(Env *env, Value object) {
    if (this->send(env, "=="_s, { object })->is_truthy()) {
        return this;
    }
    this->assert_not_frozen(env);
    if (m_klass != object->klass()) {
        env->raise("TypeError", "initialize_copy should take same class object");
    }
    return this;
}

Value KernelModule::inspect(Env *env) {
    return inspect(env, this);
}

Value KernelModule::inspect(Env *env, Value value) {
    if (value->is_module() && value->as_module()->class_name()) {
        return new StringObject { value->as_module()->class_name().value() };
    } else {
        return StringObject::format("#<{}:{}>", value->klass()->inspect_str(), value->pointer_id());
    }
}

// Note: this method is only defined here in the C++ -- the method is actually attached directly to `main` in Ruby.
Value KernelModule::main_obj_inspect(Env *env) {
    return new StringObject { "main" };
}

bool KernelModule::instance_variable_defined(Env *env, Value name_val) {
    if (is_nil() || is_boolean() || is_integer() || is_float() || is_symbol()) {
        return false;
    }
    auto name = name_val->to_instance_variable_name(env);
    return ivar_defined(env, name);
}

Value KernelModule::instance_variable_get(Env *env, Value name_val) {
    if (is_integer() || is_float()) {
        return NilObject::the();
    }
    auto name = name_val->to_instance_variable_name(env);
    return ivar_get(env, name);
}

Value KernelModule::instance_variable_set(Env *env, Value name_val, Value value) {
    this->assert_not_frozen(env);
    auto name = name_val->to_instance_variable_name(env);
    ivar_set(env, name, value);
    return value;
}

bool KernelModule::is_a(Env *env, Value module) {
    if (!module->is_module()) {
        env->raise("TypeError", "class or module required");
    }
    return Object::is_a(env, module->as_module());
}

Value KernelModule::lambda(Env *env, Block *block) {
    if (block) {
        block->set_type(Block::BlockType::Lambda);
        return new ProcObject { block };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

Value KernelModule::loop(Env *env, Block *block) {
    if (!block)
        return this->enum_for(env, "loop");

    for (;;) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, {}, nullptr);
    }
    return NilObject::the();
}

Value KernelModule::method(Env *env, Value name) {
    auto name_symbol = name->to_symbol(env, Conversion::Strict);
    auto singleton = singleton_class();
    auto module = singleton ? singleton : m_klass;
    auto method_info = module->find_method(env, name_symbol);
    if (!method_info.is_defined()) {
        auto respond_to_missing = module->find_method(env, "respond_to_missing?"_s);
        if (respond_to_missing.is_defined()) {
            if (respond_to_missing.method()->call(env, this, { name_symbol, TrueObject::the() }, nullptr)->is_truthy()) {
                auto method_missing = module->find_method(env, "method_missing"_s);
                if (method_missing.is_defined()) {
                    return new MethodObject { this, method_missing.method(), name_symbol };
                }
            }
        }
        env->raise("NoMethodError", "undefined method `{}' for {}:Class", name_symbol->inspect_str(env), m_klass->inspect_str());
    }
    return new MethodObject { this, method_info.method() };
}

Value KernelModule::methods(Env *env, Value regular_val) {
    bool regular = regular_val ? regular_val->is_truthy() : true;
    if (regular) {
        if (singleton_class()) {
            return singleton_class()->instance_methods(env, TrueObject::the());
        } else {
            return klass()->instance_methods(env, TrueObject::the());
        }
    }
    if (singleton_class()) {
        return singleton_class()->instance_methods(env, FalseObject::the());
    } else {
        return new ArrayObject {};
    }
}

Value KernelModule::p(Env *env, Args args) {
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

Value KernelModule::print(Env *env, Args args) {
    auto _stdout = env->global_get("$stdout"_s);
    assert(_stdout);
    // NATFIXME: This prevents crashes when $stdout is set to an object that
    // doesn't have a write method.  Technically this should be done when
    // setting the global, but we dont have a hook for that yet, so this will
    // do for now.
    if (!_stdout->respond_to(env, "write"_s)) {
        env->raise("TypeError", "$stdout must have write method, {} given", _stdout->klass()->inspect_str());
    }
    if (!_stdout->respond_to(env, "print"_s)) {
        env->raise("TypeError", "$stdout must have print method, {} given", _stdout->klass()->inspect_str());
    }
    // NATFIXME: Kernel.print should actually call IO.print and not
    // IO.write, but for now using IO.print causes crashes.
    // return _stdout->send(env, "print"_s, args);
    return _stdout->send(env, "write"_s, args);
}

Value KernelModule::proc(Env *env, Block *block) {
    if (block) {
        return new ProcObject { block };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

Value KernelModule::puts(Env *env, Args args) {
    auto _stdout = env->global_get("$stdout"_s);
    return _stdout->send(env, "puts"_s, args);
}

Value KernelModule::raise(Env *env, Value klass, Value message) {
    if (!klass) {
        klass = env->exception();
        if (!klass) {
            klass = find_top_level_const(env, "RuntimeError"_s);
            message = new StringObject { "" };
        }
    }
    if (!message) {
        Value arg = klass;
        if (arg->is_class()) {
            klass = arg->as_class();
            message = new StringObject { arg->as_class()->inspect_str() };
        } else if (arg->is_string()) {
            klass = find_top_level_const(env, "RuntimeError"_s)->as_class();
            message = arg;
        } else if (arg->is_exception()) {
            env->raise_exception(arg->as_exception());
        } else {
            env->raise("TypeError", "exception klass/object expected");
        }
    }
    env->raise(klass->as_class(), message->as_string());
}

Value KernelModule::Rational(Env *env, Value x, Value y, Value exception) {
    return Rational(env, x, y, exception ? exception->is_true() : true);
}

Value KernelModule::Rational(Env *env, Value x, Value y, bool exception) {
    if (y) {
        if (x->is_integer() && y->is_integer()) {
            return Rational(env, x->as_integer(), y->as_integer());
        }

        x = Float(env, x, exception);
        if (!x) {
            return nullptr;
        }

        y = Float(env, y, exception);
        if (!y) {
            return nullptr;
        }

        if (y->as_float()->is_zero())
            env->raise("ZeroDivisionError", "divided by 0");

        return Rational(env, x->as_float()->to_double() / y->as_float()->to_double());
    } else {
        if (x->is_integer()) {
            return new RationalObject { x->as_integer(), new IntegerObject { 1 } };
        }

        if (x->is_nil()) {
            if (!exception) return nullptr;
            env->raise("TypeError", "can't convert {} into Rational", x->klass()->inspect_str());
        }

        if (x->respond_to(env, "to_r"_s)) {
            auto result = x->public_send(env, "to_r"_s);
            result->assert_type(env, Object::Type::Rational, "Rational");
            return result;
        }

        x = Float(env, x, exception);
        if (!x) {
            return nullptr;
        }

        return Rational(env, x->as_float()->to_double());
    }
}

RationalObject *KernelModule::Rational(Env *env, IntegerObject *x, IntegerObject *y) {
    Value gcd = x->gcd(env, y);
    Value numerator = x->div(env, gcd);
    Value denominator = y->div(env, gcd);
    return RationalObject::create(env, numerator->as_integer(), denominator->as_integer());
}

RationalObject *KernelModule::Rational(Env *env, double arg) {
    IntegerObject radix { FLT_RADIX };
    Value y = radix.pow(env, Value::integer(DBL_MANT_DIG));

    int exponent;
    FloatObject *significand = new FloatObject { std::frexp(arg, &exponent) };
    Value x = significand->mul(env, y)->as_float()->to_i(env);

    IntegerObject two { 2 };
    if (exponent < 0) {
        y = y->as_integer()->mul(env, two.pow(env, Value::integer(-exponent)));
    } else {
        x = x->as_integer()->mul(env, two.pow(env, Value::integer(exponent)));
    }

    return Rational(env, x->as_integer(), y->as_integer());
}

Value KernelModule::remove_instance_variable(Env *env, Value name_val) {
    auto name = name_val->to_instance_variable_name(env);
    this->assert_not_frozen(env);
    return ivar_remove(env, name);
}

Value KernelModule::sleep(Env *env, Value length) {
    if (!length) {
        while (true) {
            if (FiberObject::scheduler_is_relevant())
                FiberObject::scheduler()->send(env, "kernel_sleep"_s);
            ::sleep(1000);
        }
        NAT_UNREACHABLE();
    }
    float secs;
    if (length->is_integer()) {
        secs = length->as_integer()->to_nat_int_t();
    } else if (length->is_float()) {
        secs = length->as_float()->to_double();
    } else if (length->is_rational()) {
        secs = length->as_rational()->to_f(env)->as_float()->to_double();
    } else if (length->respond_to(env, "divmod"_s)) {
        auto divmod = length->send(env, "divmod"_s, { IntegerObject::create(1) })->as_array();
        secs = divmod->at(0)->to_f(env)->as_float()->to_double();
        secs += divmod->at(1)->to_f(env)->as_float()->to_double();
    } else {
        env->raise("TypeError", "can't convert {} into time interval", length->klass()->inspect_str());
    }
    if (secs < 0.0)
        env->raise("ArgumentError", "time interval must not be negative");
    timespec ts, t_begin, t_end;
    ts.tv_sec = ::floor(secs);
    ts.tv_nsec = (secs - ts.tv_sec) * 1000000000;
    if (::clock_gettime(CLOCK_MONOTONIC, &t_begin) < 0)
        env->raise_errno();
    if (FiberObject::scheduler_is_relevant()) {
        ts.tv_sec += t_begin.tv_sec;
        ts.tv_nsec += t_begin.tv_nsec;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        while (true) {
            FiberObject::scheduler()->send(env, "kernel_sleep"_s, { length });
            // When we get here, the scheduler should have checked our timeout. But loop in case we got
            // restarted too early
            if (::clock_gettime(CLOCK_MONOTONIC, &t_end) < 0)
                env->raise_errno();
            if (t_end.tv_sec > ts.tv_sec || (t_end.tv_sec == ts.tv_sec && t_end.tv_nsec > ts.tv_nsec))
                break;
            secs = t_end.tv_sec - ts.tv_sec;
            secs += (t_end.tv_nsec - ts.tv_nsec) / 1000000000.0;
            length = new FloatObject { secs };
        }
    } else {
        nanosleep(&ts, nullptr);
        if (::clock_gettime(CLOCK_MONOTONIC, &t_end) < 0)
            env->raise_errno();
    }
    int elapsed = t_end.tv_sec - t_begin.tv_sec;
    if (t_end.tv_nsec < t_begin.tv_nsec) elapsed--;
    return IntegerObject::create(elapsed);
}

Value KernelModule::spawn(Env *env, Args args) {
    pid_t pid;
    args.ensure_argc_at_least(env, 1);
    int result;

    Vector<char *> new_env;
    if (args.size() > 1 && args.at(0)->is_hash()) {
        auto hash = args.shift()->as_hash_or_raise(env);
        for (auto ep = environ; *ep; ep++)
            new_env.push(strdup(*ep));
        for (auto pair : *hash) {
            auto combined = String::format(
                "{}={}",
                pair.key->as_string_or_raise(env)->string(),
                pair.val->as_string_or_raise(env)->string());
            new_env.push(strdup(combined.c_str()));
        }
        new_env.push(nullptr);
    }

    Defer free_new_env([&]() {
        for (auto str : new_env) {
            free(str);
        }
    });

    if (args.size() == 1) {
        auto arg = args.at(0);
        arg->assert_type(env, Object::Type::String, "String");
        auto splitter = new RegexpObject { env, "\\s+" };
        auto split = arg->as_string()->split(env, splitter, 0)->as_array();
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
    } else {
        const char *cmd[args.size() + 1];
        for (size_t i = 0; i < args.size(); i++) {
            auto arg = args[i];
            arg->assert_type(env, Object::Type::String, "String");
            cmd[i] = arg->as_string()->c_str();
        }
        cmd[args.size()] = nullptr;
        auto program = args[0]->as_string();
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
    if (value->is_string()) {
        return value;
    }

    auto to_s = "to_s"_s;

    if (!value->respond_to_method(env, to_s, true) || !value->respond_to(env, to_s)) {
        env->raise("TypeError", "can't convert {} into String", value->klass()->inspect_str());
    }

    value = value.send(env, to_s);
    value->assert_type(env, Object::Type::String, "String");
    return value;
}

Value KernelModule::tap(Env *env, Block *block) {
    Value self = this;
    Value args[] = { self };
    if (!block) {
        env->raise("LocalJumpError", "no block given (yield)");
    }
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    return this;
}

Value KernelModule::this_method(Env *env) {
    auto method = env->caller()->current_method();
    return SymbolObject::intern(method->name());
}

}
