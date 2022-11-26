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

    static Value Array(Env *env, Value value);
    static Value abort_method(Env *env, Value message);
    static Value at_exit(Env *env, Block *block);
    static Value binding(Env *env);
    static Value caller(Env *env);
    static Value Complex(Env *env, Value real, Value imaginary, Value exception);
    static Value Complex(Env *env, Value real, Value imaginary, bool exception = true);
    static Value cur_dir(Env *env);
    static Value exit(Env *env, Value status);
    static Value exit_bang(Env *env, Value status);
    static Value Integer(Env *env, Value value, Value base, Value exception);
    static Value Integer(Env *env, Value value, nat_int_t base = 0, bool exception = true);
    static Value Float(Env *env, Value value, Value exception);
    static Value Float(Env *env, Value value, bool exception = true);
    static Value fork(Env *env, Block *);
    static Value gets(Env *env);
    static Value get_usage(Env *env);
    static Value Hash(Env *env, Value value);
    static Value lambda(Env *env, Block *block);
    static Value p(Env *env, Args args);
    static Value print(Env *env, Args args);
    static Value proc(Env *env, Block *block);
    static Value puts(Env *env, Args args);
    static Value raise(Env *env, Value klass, Value message);
    static Value Rational(Env *env, Value x, Value y, Value exception);
    static Value Rational(Env *env, Value x, Value y = nullptr, bool exception = true);
    static RationalObject *Rational(Env *env, IntegerObject *x, IntegerObject *y);
    static RationalObject *Rational(Env *env, double arg);
    static Value sleep(Env *env, Value length);
    static Value spawn(Env *, Args);
    static Value String(Env *env, Value value);
    static Value this_method(Env *env);
    static bool block_given(Env *env, Block *block) { return !!block; }

    Value define_singleton_method(Env *env, Value name, Block *block);
    Value hash(Env *env);
    Value initialize_copy(Env *env, Value object);
    Value inspect(Env *env);
    static Value inspect(Env *env, Value value);
    Value main_obj_inspect(Env *);
    bool instance_variable_defined(Env *env, Value name_val);
    Value instance_variable_get(Env *env, Value name_val);
    Value instance_variable_set(Env *env, Value name_val, Value value);
    Value loop(Env *env, Block *block);
    Value method(Env *env, Value name);
    Value methods(Env *env, Value regular_val);
    Value remove_instance_variable(Env *env, Value name_val);
    Value tap(Env *env, Block *block);
    bool is_a(Env *env, Value module);
};

}
