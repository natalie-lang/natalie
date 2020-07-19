#include <sys/resource.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Kernel_puts(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *stdout = env->global_get("$stdout");
    return IO_puts(env, stdout, argc, args, block);
}

Value *Kernel_print(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *stdout = env->global_get("$stdout");
    return IO_print(env, stdout, argc, args, block);
}

Value *Kernel_p(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (argc == 0) {
        return NAT_NIL;
    } else if (argc == 1) {
        Value *arg = args[0]->send(env, "inspect");
        Kernel_puts(env, self, 1, &arg, nullptr);
        return arg;
    } else {
        ArrayValue *result = new ArrayValue { env };
        for (ssize_t i = 0; i < argc; i++) {
            result->push(args[i]);
            args[i] = args[i]->send(env, "inspect");
        }
        Kernel_puts(env, self, argc, args, nullptr);
        return result;
    }
}

Value *Kernel_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (self->is_module() && self->as_module()->class_name()) {
        return new StringValue { env, self->as_module()->class_name() };
    } else {
        StringValue *str = new StringValue { env, "#<" };
        assert(self->klass());
        StringValue *inspected = static_cast<StringValue *>(Module_inspect(env, self->klass(), 0, nullptr, nullptr));
        str->append_string(env, inspected);
        str->append_char(env, ':');
        char buf[NAT_OBJECT_POINTER_BUF_LENGTH];
        self->pointer_id(buf);
        str->append(env, buf);
        str->append_char(env, '>');
        return str;
    }
}

Value *Kernel_object_id(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new IntegerValue { env, self->object_id() };
}

Value *Kernel_equal(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (self == arg) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Kernel_class(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (self->klass()) {
        return self->klass();
    } else {
        return NAT_NIL;
    }
}

Value *Kernel_singleton_class(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self->singleton_class(env);
}

Value *Kernel_instance_variables(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    return self->ivars(env);
}

Value *Kernel_instance_variable_get(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (self->type() == Value::Type::Integer) {
        return NAT_NIL;
    }
    const char *name = args[0]->identifier_str(env, Value::Conversion::Strict);
    return self->ivar_get(env, name);
}

Value *Kernel_instance_variable_set(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    NAT_ASSERT_NOT_FROZEN(self);
    const char *name = args[0]->identifier_str(env, Value::Conversion::Strict);
    Value *val_obj = args[1];
    self->ivar_set(env, name, val_obj);
    return val_obj;
}

Value *Kernel_raise(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    ClassValue *klass;
    Value *message;
    if (argc == 2) {
        klass = args[0]->as_class();
        message = args[1];
    } else {
        Value *arg = args[0];
        if (arg->is_class()) {
            klass = arg->as_class();
            message = new StringValue { env, arg->as_class()->class_name() };
        } else if (arg->is_string()) {
            klass = NAT_OBJECT->const_get(env, "RuntimeError", true)->as_class();
            message = arg;
        } else if (arg->is_exception()) {
            env->raise_exception(arg->as_exception());
            abort();
        } else {
            NAT_RAISE(env, "TypeError", "exception klass/object expected");
        }
    }
    env->raise(klass, message->as_string()->c_str());
    abort();
}

Value *Kernel_respond_to(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    const char *name = args[0]->identifier_str(env, Value::Conversion::NullAllowed);
    if (name && self->respond_to(env, name)) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Kernel_dup(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self->dup(env);
}

Value *Kernel_methods(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0); // for now
    ArrayValue *array = new ArrayValue { env };
    if (self->singleton_class(env)) {
        self->singleton_class(env)->methods(env, array);
    } else {
        self->klass()->methods(env, array);
    }
    return array;
}

Value *Kernel_exit(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    Value *status;
    if (argc == 1) {
        status = args[0];
        if (status->type() != Value::Type::Integer) {
            status = new IntegerValue { env, 0 };
        }
    } else {
        status = new IntegerValue { env, 0 };
    }
    ExceptionValue *exception = new ExceptionValue { env, NAT_OBJECT->const_get(env, "SystemExit", true)->as_class(), "exit" };
    exception->ivar_set(env, "@status", status);
    env->raise_exception(exception);
    return NAT_NIL;
}

Value *Kernel_at_exit(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    ArrayValue *at_exit_handlers = env->global_get("$NAT_at_exit_handlers")->as_array();
    NAT_ASSERT_BLOCK();
    Value *proc = new ProcValue { env, block };
    at_exit_handlers->push(proc);
    return proc;
}

Value *Kernel_is_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *klass_or_module = args[0];
    if (!klass_or_module->is_module()) {
        NAT_RAISE(env, "TypeError", "class or module required");
    }
    if (self->is_a(env, klass_or_module->as_module())) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Kernel_hash(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    StringValue *inspected = self->send(env, "inspect")->as_string();
    ssize_t hash_value = hashmap_hash_string(inspected->c_str());
    return new IntegerValue { env, hash_value };
}

Value *Kernel_proc(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (block) {
        return new ProcValue { env, block };
    } else {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
}

Value *Kernel_lambda(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (block) {
        return new ProcValue(env, block, ProcValue::ProcType::Lambda);
    } else {
        NAT_RAISE(env, "ArgumentError", "tried to create Proc object without a block");
    }
}

Value *Kernel_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    const char *name = env->caller->find_current_method_name();
    if (name) {
        return SymbolValue::intern(env, name);
    } else {
        return NAT_NIL;
    }
}

Value *Kernel_freeze(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    self->freeze();
    return self;
}

Value *Kernel_is_nil(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return NAT_FALSE;
}

Value *Kernel_sleep(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    if (argc == 0) {
        while (true) {
            sleep(1000);
        }
        abort(); // not reached
    } else {
        Value *length = args[0];
        NAT_ASSERT_TYPE(length, Value::Type::Integer, "Integer"); // TODO: float supported also
        sleep(length->as_integer()->to_int64_t());
        return length;
    }
}

Value *Kernel_define_singleton_method(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_BLOCK();
    SymbolValue *name_obj = args[0]->to_symbol(env, Value::Conversion::Strict);
    self->define_singleton_method_with_block(env, name_obj->c_str(), block);
    return name_obj;
}

Value *Kernel_tap(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, &self, nullptr);
    return self;
}

Value *Kernel_Array(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *value = args[0];
    if (value->type() == Value::Type::Array) {
        return value;
    } else if (value->respond_to(env, "to_ary")) {
        return value->send(env, "to_ary");
    } else if (value == NAT_NIL) {
        return new ArrayValue { env };
    } else {
        ArrayValue *ary = new ArrayValue { env };
        ary->push(value);
        return ary;
    }
}

Value *Kernel_send(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC_AT_LEAST(1);
    const char *name = args[0]->identifier_str(env, Value::Conversion::Strict);
    return self->send(env->caller, name, argc - 1, args + 1, block);
}

Value *Kernel_cur_dir(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    if (env->file == nullptr) {
        NAT_RAISE(env, "RuntimeError", "could not get current directory");
    } else if (strcmp(env->file, "-e") == 0) {
        return new StringValue { env, "." };
    } else {
        Value *relative = new StringValue { env, env->file };
        StringValue *absolute = static_cast<StringValue *>(File_expand_path(env, NAT_OBJECT->const_get(env, "File", true), 1, &relative, nullptr));
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

Value *Kernel_get_usage(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return NAT_NIL;
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

}
