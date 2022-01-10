#include "natalie.hpp"

#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

extern char **environ;

namespace Natalie {

Value KernelModule::Array(Env *env, Value value) {
    if (value->is_array()) {
        return value;
    }

    if (value->respond_to(env, "to_ary"_s)) {
        auto array = value.send(env, "to_ary"_s);
        if (!array->is_nil()) {
            array->assert_type(env, Object::Type::Array, "Array");
            return array;
        }
    }

    if (value->respond_to(env, "to_a"_s)) {
        auto array = value.send(env, "to_a"_s);
        if (!array->is_nil()) {
            array->assert_type(env, Object::Type::Array, "Array");
            return array;
        }
    }

    return new ArrayObject { value };
}

Value KernelModule::abort_method(Env *env, Value message) {
    auto SystemExit = GlobalEnv::the()->Object()->const_fetch("SystemExit"_s);
    ExceptionObject *exception;

    if (message) {
        auto to_str = "to_str"_s;
        if (!message->is_string() && message->respond_to(env, to_str))
            message = message->send(env, to_str);

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

Value KernelModule::clone(Env *env) {
    auto duplicate = this->dup(env);
    auto s_class = singleton_class();
    if (s_class) {
        auto singleton_methods = new ArrayObject {};
        s_class->methods(env, singleton_methods);

        for (auto &method_name : *singleton_methods) {
            auto m = s_class->find_method(env, method_name->as_symbol());
            duplicate->singleton_class()->define_method(
                m->env(),
                SymbolObject::intern(m->name()),
                m->fn(),
                m->arity());
        }
    }
    return duplicate;
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
    if (!status || status->type() != Object::Type::Integer) {
        status = Value::integer(0);
    }
    ExceptionObject *exception = new ExceptionObject { find_top_level_const(env, "SystemExit"_s)->as_class(), new StringObject { "exit" } };
    exception->ivar_set(env, "@status"_s, status);
    env->raise_exception(exception);
    return NilObject::the();
}

Value KernelModule::Float(Env *env, Value value, Value kwargs) {
    bool exception = true;
    // NATFIXME: Improve keyword argument handling.
    if (kwargs) {
        if (kwargs->is_hash()) {
            auto value = kwargs->as_hash()->get(env, "exception"_s);
            if (value) exception = value->is_true();
        }
    }

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

    if (!value->respond_to(env, "to_hash"_s)) {
        env->raise("TypeError", "can't convert {} into Hash", value->klass()->inspect_str());
    }

    value = value.send(env, "to_hash"_s);
    value->assert_type(env, Object::Type::Hash, "Hash");
    return value;
}

Value KernelModule::hash(Env *env) {
    switch (type()) {
    // NOTE: string "foo" and symbol :foo will get the same hash.
    // That's probably ok, but maybe worth revisiting.
    case Type::String:
        return Value::integer(TM::Hashmap<void *>::hash_str(as_string()->c_str()));
    case Type::Symbol:
        return Value::integer(TM::Hashmap<void *>::hash_str(as_symbol()->c_str()));
    default: {
        StringObject *inspected = send(env, "inspect"_s)->as_string();
        nat_int_t hash_value = TM::Hashmap<void *>::hash_str(inspected->c_str());
        return Value::integer(hash_value);
    }
    }
}

Value KernelModule::inspect(Env *env) {
    return inspect(env, this);
}

Value KernelModule::inspect(Env *env, Value value) {
    if (value->is_module() && value->as_module()->class_name()) {
        return new StringObject { *value->as_module()->class_name().value() };
    } else {
        return StringObject::format(env, "#<{}:{}>", value->klass()->inspect_str(), value->pointer_id());
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
        return new ProcObject { block, ProcObject::ProcType::Lambda };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

Value KernelModule::loop(Env *env, Block *block) {
    if (!block)
        return this->enum_for(env, "loop");

    for (;;) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    }
    return NilObject::the();
}

Value KernelModule::method(Env *env, Value name) {
    auto name_symbol = name->to_symbol(env, Conversion::Strict);
    auto singleton = singleton_class();
    if (singleton) {
        Method *method = singleton_class()->find_method(env, name_symbol);
        if (method) {
            if (method->is_undefined()) {
                env->raise("NoMethodError", "undefined method `{}' for {}:Class", name_symbol->inspect_str(env), m_klass->inspect_str());
            }
            return new MethodObject { this, method };
        }
    }
    Method *method = m_klass->find_method(env, name_symbol);
    if (method && !method->is_undefined())
        return new MethodObject { this, method };
    env->raise("NoMethodError", "undefined method `{}' for {}:Class", name_symbol->inspect_str(env), m_klass->inspect_str());
}

Value KernelModule::methods(Env *env) {
    ArrayObject *array = new ArrayObject {};
    if (singleton_class()) {
        singleton_class()->methods(env, array);
    } else {
        klass()->methods(env, array);
    }
    return array;
}

Value KernelModule::p(Env *env, size_t argc, Value *args) {
    if (argc == 0) {
        return NilObject::the();
    } else if (argc == 1) {
        Value arg = args[0].send(env, "inspect"_s);
        puts(env, 1, &arg);
        return arg;
    } else {
        ArrayObject *result = new ArrayObject { argc };
        for (size_t i = 0; i < argc; i++) {
            result->push(args[i]);
            args[i] = args[i].send(env, "inspect"_s);
        }
        puts(env, argc, args);
        return result;
    }
}

Value KernelModule::print(Env *env, size_t argc, Value *args) {
    IoObject *_stdout = env->global_get("$stdout"_s)->as_io();
    return _stdout->print(env, argc, args);
}

Value KernelModule::proc(Env *env, Block *block) {
    if (block) {
        return new ProcObject { block };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

Value KernelModule::puts(Env *env, size_t argc, Value *args) {
    IoObject *_stdout = env->global_get("$stdout"_s)->as_io();
    return _stdout->puts(env, argc, args);
}

Value KernelModule::raise(Env *env, Value klass, Value message) {
    if (!message) {
        Value arg = klass;
        if (arg->is_class()) {
            klass = arg->as_class();
            message = new StringObject { *arg->as_class()->inspect_str() };
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

Value KernelModule::remove_instance_variable(Env *env, Value name_val) {
    auto name = name_val->to_instance_variable_name(env);
    return ivar_remove(env, name);
}

Value KernelModule::sleep(Env *env, Value length) {
    if (!length) {
        while (true) {
            ::sleep(1000);
        }
        NAT_UNREACHABLE();
    }
    if (length->is_integer()) {
        auto secs = length->as_integer()->to_nat_int_t();
        if (secs < 0)
            env->raise("ArgumentError", "time interval must not be negative");
        ::sleep(length->as_integer()->to_nat_int_t());
    } else if (length->is_float()) {
        auto secs = length->as_float()->to_double();
        if (secs < 0.0)
            env->raise("ArgumentError", "time interval must not be negative");
        struct timespec ts;
        ts.tv_sec = ::floor(secs);
        ts.tv_nsec = (secs - ts.tv_sec) * 1000000000;
        nanosleep(&ts, nullptr);
    } else {
        env->raise("TypeError", "can't convert {} into time interval", length->klass()->inspect_str());
    }
    return length;
}

Value KernelModule::spawn(Env *env, size_t argc, Value *args) {
    pid_t pid;
    env->ensure_argc_at_least(argc, 1);
    auto program = args[0]->as_string();
    char *cmd[argc + 1];
    for (size_t i = 0; i < argc; i++) {
        auto arg = args[i];
        arg->assert_type(env, Object::Type::String, "String");
        cmd[i] = strdup(arg->as_string()->c_str());
    }
    cmd[argc] = nullptr;
    int result = posix_spawnp(&pid, program->c_str(), NULL, NULL, cmd, environ);
    for (size_t i = 0; i < argc; i++) {
        free(cmd[i]);
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
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, nullptr);
    return this;
}

Value KernelModule::this_method(Env *env) {
    auto method = env->caller()->current_method();
    if (method->name()) {
        return SymbolObject::intern(method->name());
    } else {
        return NilObject::the();
    }
}
}
