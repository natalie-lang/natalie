#include "natalie.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>

namespace Natalie {

Value *KernelModule::Array(Env *env, Value *value) {
    if (value->type() == Value::Type::Array) {
        return value;
    } else if (value->respond_to(env, "to_ary")) {
        return value->send(env, "to_ary");
    } else if (value == env->nil_obj()) {
        return new ArrayValue { env };
    } else {
        ArrayValue *ary = new ArrayValue { env };
        ary->push(value);
        return ary;
    }
}

Value *KernelModule::at_exit(Env *env, Block *block) {
    ArrayValue *at_exit_handlers = env->global_get("$NAT_at_exit_handlers")->as_array();
    NAT_ASSERT_BLOCK();
    Value *proc = new ProcValue { env, block };
    at_exit_handlers->push(proc);
    return proc;
}

Value *KernelModule::cur_dir(Env *env) {
    if (env->file() == nullptr) {
        env->raise("RuntimeError", "could not get current directory");
    } else if (strcmp(env->file(), "-e") == 0) {
        return new StringValue { env, "." };
    } else {
        Value *relative = new StringValue { env, env->file() };
        StringValue *absolute = static_cast<StringValue *>(FileValue::expand_path(env, relative, nullptr));
        ssize_t last_slash = 0;
        bool found = false;
        for (ssize_t i = 0; i < absolute->length(); i++) {
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

Value *KernelModule::define_singleton_method(Env *env, Value *name, Block *block) {
    NAT_ASSERT_BLOCK();
    SymbolValue *name_obj = name->to_symbol(env, Value::Conversion::Strict);
    define_singleton_method_with_block(env, name_obj->c_str(), block);
    return name_obj;
}

Value *KernelModule::exit(Env *env, Value *status) {
    if (!status || status->type() != Value::Type::Integer) {
        status = new IntegerValue { env, 0 };
    }
    ExceptionValue *exception = new ExceptionValue { env, env->Object()->const_find(env, "SystemExit")->as_class(), "exit" };
    exception->ivar_set(env, "@status", status);
    env->raise_exception(exception);
    return env->nil_obj();
}

Value *KernelModule::get_usage(Env *env) {
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

Value *KernelModule::hash(Env *env) {
    StringValue *inspected = send(env, "inspect")->as_string();
    ssize_t hash_value = hashmap_hash_string(inspected->c_str());
    return new IntegerValue { env, hash_value };
}

Value *KernelModule::inspect(Env *env) {
    if (is_module() && as_module()->class_name()) {
        return new StringValue { env, as_module()->class_name() };
    } else {
        StringValue *str = new StringValue { env, "#<" };
        assert(klass());
        StringValue *inspected = klass()->send(env, "inspect")->as_string();
        str->append_string(env, inspected);
        str->append_char(env, ':');
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        pointer_id(buf);
        str->append(env, buf);
        str->append_char(env, '>');
        return str;
    }
}

// Note: this method is only defined here in the C++ -- the method is actually attached directly to `main` in Ruby.
Value *KernelModule::main_obj_inspect(Env *env) {
    return new StringValue { env, "main" };
}

Value *KernelModule::instance_variable_get(Env *env, Value *name_val) {
    if (is_integer() || is_float()) {
        return env->nil_obj();
    }
    const char *name = name_val->identifier_str(env, Value::Conversion::Strict);
    return ivar_get(env, name);
}

Value *KernelModule::instance_variable_set(Env *env, Value *name_val, Value *value) {
    NAT_ASSERT_NOT_FROZEN(this);
    const char *name = name_val->identifier_str(env, Value::Conversion::Strict);
    ivar_set(env, name, value);
    return value;
}

bool KernelModule::is_a(Env *env, Value *module) {
    if (!module->is_module()) {
        env->raise("TypeError", "class or module required");
    }
    return Value::is_a(env, module->as_module());
}

Value *KernelModule::lambda(Env *env, Block *block) {
    if (block) {
        return new ProcValue(env, block, ProcValue::ProcType::Lambda);
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

Value *KernelModule::loop(Env *env, Block *block) {
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

Value *KernelModule::methods(Env *env) {
    ArrayValue *array = new ArrayValue { env };
    if (singleton_class()) {
        singleton_class()->methods(env, array);
    } else { // FIXME: I don't think this else should be here. Shouldn't we *always* grab methods from the klass?
        klass()->methods(env, array);
    }
    return array;
}

Value *KernelModule::p(Env *env, ssize_t argc, Value **args) {
    if (argc == 0) {
        return env->nil_obj();
    } else if (argc == 1) {
        Value *arg = args[0]->send(env, "inspect");
        puts(env, 1, &arg);
        return arg;
    } else {
        ArrayValue *result = new ArrayValue { env };
        for (ssize_t i = 0; i < argc; i++) {
            result->push(args[i]);
            args[i] = args[i]->send(env, "inspect");
        }
        puts(env, argc, args);
        return result;
    }
}

Value *KernelModule::print(Env *env, ssize_t argc, Value **args) {
    IoValue *stdout = env->global_get("$stdout")->as_io();
    return stdout->print(env, argc, args);
}

Value *KernelModule::proc(Env *env, Block *block) {
    if (block) {
        return new ProcValue { env, block };
    } else {
        env->raise("ArgumentError", "tried to create Proc object without a block");
    }
}

Value *KernelModule::puts(Env *env, ssize_t argc, Value **args) {
    IoValue *stdout = env->global_get("$stdout")->as_io();
    return stdout->puts(env, argc, args);
}

Value *KernelModule::raise(Env *env, Value *klass, Value *message) {
    if (!message) {
        Value *arg = klass;
        if (arg->is_class()) {
            klass = arg->as_class();
            message = new StringValue { env, arg->as_class()->class_name() };
        } else if (arg->is_string()) {
            klass = env->Object()->const_find(env, "RuntimeError")->as_class();
            message = arg;
        } else if (arg->is_exception()) {
            env->raise_exception(arg->as_exception());
        } else {
            env->raise("TypeError", "exception klass/object expected");
        }
    }
    env->raise(klass->as_class(), message->as_string()->c_str());
}

Value *KernelModule::sleep(Env *env, Value *length) {
    if (!length) {
        while (true) {
            ::sleep(1000);
        }
        NAT_UNREACHABLE();
    }
    length->assert_type(env, Value::Type::Integer, "Integer"); // TODO: float supported also
    ::sleep(length->as_integer()->to_int64_t());
    return length;
}

Value *KernelModule::tap(Env *env, Block *block) {
    Value *self = this;
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, nullptr);
    return this;
}

Value *KernelModule::this_method(Env *env) {
    const char *name = env->caller()->find_current_method_name();
    if (name) {
        return SymbolValue::intern(env, name);
    } else {
        return env->nil_obj();
    }
}

}
