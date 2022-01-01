#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/nil_object.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class KernelModule : public Object {
public:
    Value object_id(Env *env) {
        return Value::integer(Object::object_id());
    }

    bool equal(Value other) {
        return other == this;
    }

    Value klass_obj(Env *env) {
        if (klass()) {
            return klass();
        } else {
            return NilObject::the();
        }
    }

    Value singleton_class_obj(Env *env) {
        return singleton_class(env);
    }

    Value freeze_obj(Env *env) {
        freeze();
        return this;
    }

    Value Array(Env *env, Value value);
    Value abort_method(Env *env, Value message);
    Value at_exit(Env *env, Block *block);
    Value binding(Env *env);
    Value clone(Env *env);
    Value cur_dir(Env *env);
    Value define_singleton_method(Env *env, Value name, Block *block);
    Value exit(Env *env, Value status);
    Value Float(Env *env, Value value, Value kwargs = nullptr);
    Value gets(Env *env);
    Value get_usage(Env *env);
    Value Hash(Env *env, Value value);
    Value hash(Env *env);
    Value inspect(Env *env);
    static Value inspect(Env *env, Value value);
    Value main_obj_inspect(Env *);
    bool instance_variable_defined(Env *env, Value name_val);
    Value instance_variable_get(Env *env, Value name_val);
    Value instance_variable_set(Env *env, Value name_val, Value value);
    Value lambda(Env *env, Block *block);
    Value loop(Env *env, Block *block);
    Value method(Env *env, Value name);
    Value methods(Env *env);
    Value p(Env *env, size_t argc, Value *args);
    Value print(Env *env, size_t argc, Value *args);
    Value proc(Env *env, Block *block);
    Value puts(Env *env, size_t argc, Value *args);
    Value raise(Env *env, Value klass, Value message);
    Value remove_instance_variable(Env *env, Value name_val);
    Value sleep(Env *env, Value length);
    Value spawn(Env *, size_t, Value *);
    Value String(Env *env, Value value);
    Value tap(Env *env, Block *block);
    Value this_method(Env *env);
    bool is_a(Env *env, Value module);
    bool block_given(Env *env, Block *block) { return !!block; }
};

}
