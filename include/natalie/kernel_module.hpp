#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/value.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/nil_value.hpp"

namespace Natalie {

struct KernelModule : Value {
    Value *object_id(Env *env) {
        return new IntegerValue { env, Value::object_id() };
    }

    bool equal(Value *other) {
        return this == other;
    }

    Value *klass_obj(Env *env) {
        if (klass()) {
            return klass();
        } else {
            return env->nil_obj();
        }
    }

    Value *singleton_class_obj(Env *env) {
        return singleton_class(env);
    }

    Value *freeze_obj(Env *env) {
        freeze();
        return this;
    }

    Value *Array(Env *env, Value *value);
    Value *at_exit(Env *env, Block *block);
    Value *cur_dir(Env *env);
    Value *define_singleton_method(Env *env, Value *name, Block *block);
    Value *exit(Env *env, Value *status);
    Value *get_usage(Env *env);
    Value *hash(Env *env);
    Value *inspect(Env *env);
    Value *main_obj_inspect(Env *);
    Value *instance_variable_get(Env *env, Value *name_val);
    Value *instance_variable_set(Env *env, Value *name_val, Value *value);
    Value *lambda(Env *env, Block *block);
    Value *loop(Env *env, Block *block);
    Value *methods(Env *env);
    Value *p(Env *env, ssize_t argc, Value **args);
    Value *print(Env *env, ssize_t argc, Value **args);
    Value *proc(Env *env, Block *block);
    Value *puts(Env *env, ssize_t argc, Value **args);
    Value *raise(Env *env, Value *klass, Value *message);
    Value *sleep(Env *env, Value *length);
    Value *tap(Env *env, Block *block);
    Value *this_method(Env *env);
    bool is_a(Env *env, Value *module);
    bool block_given(Env *env, Block *block) { return !!block; }
};

}
