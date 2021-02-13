#include "natalie.hpp"

#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

extern char **environ;

namespace Natalie {

ValuePtr KernelModule::Array(Env *env, ValuePtr value) {
    if (value->type() == Value::Type::Array) {
        return value;
    } else if (value->respond_to(env, "to_ary")) {
        return value.send(env, "to_ary");
    } else if (value == env->nil_obj()) {
        return new ArrayValue { env };
    } else {
        ArrayValue *ary = new ArrayValue { env };
        ary->push(value);
        return ary;
    }
}

ValuePtr KernelModule::at_exit(Env *env, Block *block) {
    ArrayValue *at_exit_handlers = env->global_get(SymbolValue::intern(env, "$NAT_at_exit_handlers"))->as_array();
    env->assert_block_given(block);
    ValuePtr proc = new ProcValue { env, block };
    at_exit_handlers->push(proc);
    return proc;
}

ValuePtr KernelModule::cur_dir(Env *env) {
    if (env->file() == nullptr) {
        env->raise("RuntimeError", "could not get current directory");
    } else if (strcmp(env->file(), "-e") == 0) {
        return new StringValue { env, "." };
    } else {
        ValuePtr relative = new StringValue { env, env->file() };
        StringValue *absolute = FileValue::expand_path(env, relative, nullptr)->as_string();
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
    env->assert_block_given(block);
    SymbolValue *name_obj = name->to_symbol(env, Value::Conversion::Strict);
    define_singleton_method_with_block(env, name_obj, block);
    return name_obj;
}

ValuePtr KernelModule::exit(Env *env, ValuePtr status) {
    if (!status || status->type() != Value::Type::Integer) {
        status = new IntegerValue { env, 0 };
    }
    ExceptionValue *exception = new ExceptionValue { env, env->Object()->const_find(env, SymbolValue::intern(env, "SystemExit"))->as_class(), "exit" };
    exception->ivar_set(env, SymbolValue::intern(env, "@status"), status);
    env->raise_exception(exception);
    return env->nil_obj();
}

ValuePtr KernelModule::get_usage(Env *env) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return env->nil_obj();
    }
    HashValue *hash = new HashValue { env };
    hash->put(env, new StringValue { env, "maxrss" }, new IntegerValue { env, usage.ru_maxrss });
    hash->put(env, new StringValue { env, "ixrss" }, new IntegerValue { env, usage.ru_ixrss });
    hash->put(env, new StringValue { env, "idrss" }, new IntegerValue { env, usage.ru_idrss });
    hash->put(env, new StringValue { env, "isrss" }, new IntegerValue { env, usage.ru_isrss });
    hash->put(env, new StringValue { env, "minflt" }, new IntegerValue { env, usage.ru_minflt });
    hash->put(env, new StringValue { env, "majflt" }, new IntegerValue { env, usage.ru_majflt });
    hash->put(env, new StringValue { env, "nswap" }, new IntegerValue { env, usage.ru_nswap });
    hash->put(env, new StringValue { env, "inblock" }, new IntegerValue { env, usage.ru_inblock });
    hash->put(env, new StringValue { env, "oublock" }, new IntegerValue { env, usage.ru_oublock });
    hash->put(env, new StringValue { env, "msgsnd" }, new IntegerValue { env, usage.ru_msgsnd });
    hash->put(env, new StringValue { env, "msgrcv" }, new IntegerValue { env, usage.ru_msgrcv });
    hash->put(env, new StringValue { env, "nsignals" }, new IntegerValue { env, usage.ru_nsignals });
    hash->put(env, new StringValue { env, "nvcsw" }, new IntegerValue { env, usage.ru_nvcsw });
    hash->put(env, new StringValue { env, "nivcsw" }, new IntegerValue { env, usage.ru_nivcsw });
    return hash;
}

ValuePtr KernelModule::hash(Env *env) {
    StringValue *inspected = _send(env, "inspect")->as_string();
    nat_int_t hash_value = hashmap_hash_string(inspected->c_str());
    return new IntegerValue { env, hash_value };
}

ValuePtr KernelModule::inspect(Env *env) {
    if (is_module() && as_module()->class_name()) {
        return new StringValue { env, as_module()->class_name() };
    } else {
        return StringValue::sprintf(env, "#<%s:%s>", klass()->inspect_str(env), pointer_id());
    }
}

// Note: this method is only defined here in the C++ -- the method is actually attached directly to `main` in Ruby.
ValuePtr KernelModule::main_obj_inspect(Env *env) {
    return new StringValue { env, "main" };
}

ValuePtr KernelModule::instance_variable_get(Env *env, ValuePtr name_val) {
    if (is_integer() || is_float()) {
        return env->nil_obj();
    }
    auto name = name_val->to_symbol(env, Value::Conversion::Strict);
    return ivar_get(env, name);
}

ValuePtr KernelModule::instance_variable_set(Env *env, ValuePtr name_val, ValuePtr value) {
    this->assert_not_frozen(env);
    auto name = name_val->to_symbol(env, Value::Conversion::Strict);
    ivar_set(env, name, value);
    return value;
}

bool KernelModule::is_a(Env *env, ValuePtr module) {
    if (!module->is_module()) {
        env->raise("TypeError", "class or module required");
    }
    return Value::is_a(env, module->as_module());
}

ValuePtr KernelModule::lambda(Env *env, Block *block) {
    if (block) {
        return new ProcValue(env, block, ProcValue::ProcType::Lambda);
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

ValuePtr KernelModule::loop(Env *env, Block *block) {
    if (block) {
        for (;;) {
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
        }
        return env->nil_obj();
    } else {
        // TODO: Enumerator?
        env->raise("ArgumentError", "loop without block");
    }
}

ValuePtr KernelModule::method(Env *env, ValuePtr name) {
    auto name_symbol = name->to_symbol(env, Conversion::Strict);
    auto singleton = singleton_class();
    if (singleton) {
        Method *method = singleton_class()->find_method(env, name_symbol->c_str());
        if (method) {
            if (method->is_undefined()) {
                env->raise("NoMethodError", "undefined method `%s' for %s:Class", name, m_klass->class_name());
            }
            return new MethodValue { env, this, method };
        }
    }
    Method *method = m_klass->find_method(env, name_symbol->c_str());
    if (method)
        return new MethodValue { env, this, method };
    env->raise("NoMethodError", "undefined method `%s' for %s:Class", name, m_klass->class_name());
}

ValuePtr KernelModule::methods(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    if (singleton_class()) {
        singleton_class()->methods(env, array);
    } else { // FIXME: I don't think this else should be here. Shouldn't we *always* grab methods from the klass?
        klass()->methods(env, array);
    }
    return array;
}

ValuePtr KernelModule::p(Env *env, size_t argc, ValuePtr *args) {
    if (argc == 0) {
        return env->nil_obj();
    } else if (argc == 1) {
        ValuePtr arg = args[0].send(env, "inspect");
        puts(env, 1, &arg);
        return arg;
    } else {
        ArrayValue *result = new ArrayValue { env };
        for (size_t i = 0; i < argc; i++) {
            result->push(args[i]);
            args[i] = args[i].send(env, "inspect");
        }
        puts(env, argc, args);
        return result;
    }
}

ValuePtr KernelModule::print(Env *env, size_t argc, ValuePtr *args) {
    IoValue *_stdout = env->global_get(SymbolValue::intern(env, "$stdout"))->as_io();
    return _stdout->print(env, argc, args);
}

ValuePtr KernelModule::proc(Env *env, Block *block) {
    if (block) {
        return new ProcValue { env, block };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

ValuePtr KernelModule::puts(Env *env, size_t argc, ValuePtr *args) {
    IoValue *_stdout = env->global_get(SymbolValue::intern(env, "$stdout"))->as_io();
    return _stdout->puts(env, argc, args);
}

ValuePtr KernelModule::raise(Env *env, ValuePtr klass, ValuePtr message) {
    if (!message) {
        ValuePtr arg = klass;
        if (arg->is_class()) {
            klass = arg->as_class();
            message = new StringValue { env, arg->as_class()->class_name() };
        } else if (arg->is_string()) {
            klass = env->Object()->const_find(env, SymbolValue::intern(env, "RuntimeError"))->as_class();
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
    length->assert_type(env, Value::Type::Integer, "Integer"); // TODO: float supported also
    ::sleep(length->as_integer()->to_nat_int_t());
    return length;
}

ValuePtr KernelModule::spawn(Env *env, size_t argc, ValuePtr *args) {
    pid_t pid;
    auto command = new Vector<const char *> {};
    char *cmd[argc + 1];
    for (size_t i = 0; i < argc; i++) {
        auto arg = args[i];
        arg->assert_type(env, Value::Type::String, "String");
        cmd[i] = GC_STRDUP(arg->as_string()->c_str());
    }
    cmd[argc] = nullptr;
    int result = posix_spawnp(&pid, cmd[0], NULL, NULL, cmd, environ);
    if (result == 0) {
        return new IntegerValue { env, pid };
    } else {
        env->raise_errno();
    }
}

ValuePtr KernelModule::tap(Env *env, Block *block) {
    ValuePtr self = this;
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, nullptr);
    return this;
}

ValuePtr KernelModule::this_method(Env *env) {
    const char *name = env->caller()->find_current_method_name();
    if (name) {
        return SymbolValue::intern(env, name);
    } else {
        return env->nil_obj();
    }
}
}
