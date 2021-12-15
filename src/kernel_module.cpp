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

ValuePtr KernelModule::Array(Env *env, ValuePtr value) {
    if (value->type() == Object::Type::Array) {
        return value;
    } else if (value->respond_to(env, SymbolObject::intern("to_ary"))) {
        return value.send(env, SymbolObject::intern("to_ary"));
    } else if (value == NilObject::the()) {
        return new ArrayObject {};
    } else {
        ArrayObject *ary = new ArrayObject {};
        ary->push(value);
        return ary;
    }
}

ValuePtr KernelModule::at_exit(Env *env, Block *block) {
    ArrayObject *at_exit_handlers = env->global_get(SymbolObject::intern("$NAT_at_exit_handlers"))->as_array();
    env->ensure_block_given(block);
    ValuePtr proc = new ProcObject { block };
    at_exit_handlers->push(proc);
    return proc;
}

ValuePtr KernelModule::binding(Env *env) {
    return new BindingObject { env };
}

ValuePtr KernelModule::clone(Env *env) {
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

ValuePtr KernelModule::cur_dir(Env *env) {
    if (env->file() == nullptr) {
        env->raise("RuntimeError", "could not get current directory");
    } else if (strcmp(env->file(), "-e") == 0) {
        return new StringObject { "." };
    } else {
        ValuePtr relative = new StringObject { env->file() };
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

ValuePtr KernelModule::define_singleton_method(Env *env, ValuePtr name, Block *block) {
    env->ensure_block_given(block);
    SymbolObject *name_obj = name->to_symbol(env, Object::Conversion::Strict);
    Object::define_singleton_method(env, name_obj, block);
    return name_obj;
}

ValuePtr KernelModule::exit(Env *env, ValuePtr status) {
    if (!status || status->type() != Object::Type::Integer) {
        status = ValuePtr::integer(0);
    }
    ExceptionObject *exception = new ExceptionObject { GlobalEnv::the()->Object()->const_find(env, SymbolObject::intern("SystemExit"))->as_class(), new StringObject { "exit" } };
    exception->ivar_set(env, SymbolObject::intern("@status"), status);
    env->raise_exception(exception);
    return NilObject::the();
}

ValuePtr KernelModule::gets(Env *env) {
    char buf[2048];
    if (!fgets(buf, 2048, stdin))
        return NilObject::the();
    return new StringObject { buf };
}

ValuePtr KernelModule::get_usage(Env *env) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return NilObject::the();
    }
    HashObject *hash = new HashObject {};
    hash->put(env, new StringObject { "maxrss" }, ValuePtr::integer(usage.ru_maxrss));
    hash->put(env, new StringObject { "ixrss" }, ValuePtr::integer(usage.ru_ixrss));
    hash->put(env, new StringObject { "idrss" }, ValuePtr::integer(usage.ru_idrss));
    hash->put(env, new StringObject { "isrss" }, ValuePtr::integer(usage.ru_isrss));
    hash->put(env, new StringObject { "minflt" }, ValuePtr::integer(usage.ru_minflt));
    hash->put(env, new StringObject { "majflt" }, ValuePtr::integer(usage.ru_majflt));
    hash->put(env, new StringObject { "nswap" }, ValuePtr::integer(usage.ru_nswap));
    hash->put(env, new StringObject { "inblock" }, ValuePtr::integer(usage.ru_inblock));
    hash->put(env, new StringObject { "oublock" }, ValuePtr::integer(usage.ru_oublock));
    hash->put(env, new StringObject { "msgsnd" }, ValuePtr::integer(usage.ru_msgsnd));
    hash->put(env, new StringObject { "msgrcv" }, ValuePtr::integer(usage.ru_msgrcv));
    hash->put(env, new StringObject { "nsignals" }, ValuePtr::integer(usage.ru_nsignals));
    hash->put(env, new StringObject { "nvcsw" }, ValuePtr::integer(usage.ru_nvcsw));
    hash->put(env, new StringObject { "nivcsw" }, ValuePtr::integer(usage.ru_nivcsw));
    return hash;
}

ValuePtr KernelModule::hash(Env *env) {
    switch (type()) {
    // NOTE: string "foo" and symbol :foo will get the same hash.
    // That's probably ok, but maybe worth revisiting.
    case Type::String:
        return ValuePtr::integer(TM::Hashmap<void *>::hash_str(as_string()->c_str()));
    case Type::Symbol:
        return ValuePtr::integer(TM::Hashmap<void *>::hash_str(as_symbol()->c_str()));
    default: {
        StringObject *inspected = send(env, SymbolObject::intern("inspect"))->as_string();
        nat_int_t hash_value = TM::Hashmap<void *>::hash_str(inspected->c_str());
        return ValuePtr::integer(hash_value);
    }
    }
}

ValuePtr KernelModule::inspect(Env *env) {
    if (is_module() && as_module()->class_name()) {
        return new StringObject { *as_module()->class_name().value() };
    } else {
        return StringObject::format(env, "#<{}:{}>", klass()->inspect_str(env), pointer_id());
    }
}

// Note: this method is only defined here in the C++ -- the method is actually attached directly to `main` in Ruby.
ValuePtr KernelModule::main_obj_inspect(Env *env) {
    return new StringObject { "main" };
}

ValuePtr KernelModule::instance_variable_get(Env *env, ValuePtr name_val) {
    if (is_integer() || is_float()) {
        return NilObject::the();
    }
    auto name = name_val->to_symbol(env, Object::Conversion::Strict);
    return ivar_get(env, name);
}

ValuePtr KernelModule::instance_variable_set(Env *env, ValuePtr name_val, ValuePtr value) {
    this->assert_not_frozen(env);
    auto name = name_val->to_symbol(env, Object::Conversion::Strict);
    ivar_set(env, name, value);
    return value;
}

bool KernelModule::is_a(Env *env, ValuePtr module) {
    if (!module->is_module()) {
        env->raise("TypeError", "class or module required");
    }
    return Object::is_a(env, module->as_module());
}

ValuePtr KernelModule::lambda(Env *env, Block *block) {
    if (block) {
        return new ProcObject { block, ProcObject::ProcType::Lambda };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

ValuePtr KernelModule::loop(Env *env, Block *block) {
    if (!block)
        return this->enum_for(env, "loop");

    for (;;) {
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
    }
    return NilObject::the();
}

ValuePtr KernelModule::method(Env *env, ValuePtr name) {
    auto name_symbol = name->to_symbol(env, Conversion::Strict);
    auto singleton = singleton_class();
    if (singleton) {
        Method *method = singleton_class()->find_method(env, name_symbol);
        if (method) {
            if (method->is_undefined()) {
                env->raise("NoMethodError", "undefined method `{}' for {}:Class", name_symbol->inspect_str(env), m_klass->class_name_or_blank());
            }
            return new MethodObject { this, method };
        }
    }
    Method *method = m_klass->find_method(env, name_symbol);
    if (method)
        return new MethodObject { this, method };
    env->raise("NoMethodError", "undefined method `{}' for {}:Class", name_symbol->inspect_str(env), m_klass->class_name_or_blank());
}

ValuePtr KernelModule::methods(Env *env) {
    ArrayObject *array = new ArrayObject {};
    if (singleton_class()) {
        singleton_class()->methods(env, array);
    } else {
        klass()->methods(env, array);
    }
    return array;
}

ValuePtr KernelModule::p(Env *env, size_t argc, ValuePtr *args) {
    if (argc == 0) {
        return NilObject::the();
    } else if (argc == 1) {
        ValuePtr arg = args[0].send(env, SymbolObject::intern("inspect"));
        puts(env, 1, &arg);
        return arg;
    } else {
        ArrayObject *result = new ArrayObject {};
        for (size_t i = 0; i < argc; i++) {
            result->push(args[i]);
            args[i] = args[i].send(env, SymbolObject::intern("inspect"));
        }
        puts(env, argc, args);
        return result;
    }
}

ValuePtr KernelModule::print(Env *env, size_t argc, ValuePtr *args) {
    IoObject *_stdout = env->global_get(SymbolObject::intern("$stdout"))->as_io();
    return _stdout->print(env, argc, args);
}

ValuePtr KernelModule::proc(Env *env, Block *block) {
    if (block) {
        return new ProcObject { block };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

ValuePtr KernelModule::puts(Env *env, size_t argc, ValuePtr *args) {
    IoObject *_stdout = env->global_get(SymbolObject::intern("$stdout"))->as_io();
    return _stdout->puts(env, argc, args);
}

ValuePtr KernelModule::raise(Env *env, ValuePtr klass, ValuePtr message) {
    if (!message) {
        ValuePtr arg = klass;
        if (arg->is_class()) {
            klass = arg->as_class();
            message = new StringObject { *arg->as_class()->class_name_or_blank() };
        } else if (arg->is_string()) {
            klass = GlobalEnv::the()->Object()->const_find(env, SymbolObject::intern("RuntimeError"))->as_class();
            message = arg;
        } else if (arg->is_exception()) {
            env->raise_exception(arg->as_exception());
        } else {
            env->raise("TypeError", "exception klass/object expected");
        }
    }
    env->raise(klass->as_class(), message->as_string());
}

ValuePtr KernelModule::sleep(Env *env, ValuePtr length) {
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
        env->raise("TypeError", "can't convert {} into time interval", length->klass()->inspect_str(env));
    }
    return length;
}

ValuePtr KernelModule::spawn(Env *env, size_t argc, ValuePtr *args) {
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
    return ValuePtr::integer(pid);
}

ValuePtr KernelModule::tap(Env *env, Block *block) {
    ValuePtr self = this;
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, nullptr);
    return this;
}

ValuePtr KernelModule::this_method(Env *env) {
    auto method = env->caller()->current_method();
    if (method->name()) {
        return SymbolObject::intern(method->name());
    } else {
        return NilObject::the();
    }
}
}
