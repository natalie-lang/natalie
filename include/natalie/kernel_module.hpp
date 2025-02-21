#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class KernelModule {
public:
    // module functions
    static Value Array(Env *env, Value value);
    static Value abort_method(Env *env, Value message);
    static Value at_exit(Env *env, Block *block);
    static Value backtick(Env *, Value);
    static Value binding(Env *env);
    static bool block_given(Env *env, Block *block) { return !!block; }
    static Value caller(Env *env, Value start = nullptr, Value length = nullptr);
    static Value caller_locations(Env *env, Value start = nullptr, Value length = nullptr);
    static Value catch_method(Env *, Value = nullptr, Block * = nullptr);
    static Value Complex(Env *env, Value real, Value imaginary, Value exception);
    static Value Complex(Env *env, Value real, Value imaginary, bool exception = true);
    static Value Complex(Env *env, StringObject *input, bool exception, bool string_to_c);
    static Value cur_callee(Env *env);
    static Value cur_dir(Env *env);
    static Value exit(Env *env, Value status);
    static Value exit_bang(Env *env, Value status);
    static Value Integer(Env *env, Value value, Value base, Value exception);
    static Value Integer(Env *env, Value value, nat_int_t base = 0, bool exception = true);
    static bool is_nil(Value value) { return value.is_nil(); }
    static Value Float(Env *env, Value value, Value exception);
    static Value Float(Env *env, Value value, bool exception = true);
    static Value fork(Env *env, Block *);
    static Value gets(Env *env);
    static Value get_usage(Env *env);
    static Value global_variables(Env *env);
    static Value Hash(Env *env, Value value);
    static Value lambda(Env *env, Block *block);
    static Value p(Env *env, Args &&args);
    static Value print(Env *env, Args &&args);
    static Value proc(Env *env, Block *block);
    static Value puts(Env *env, Args &&args);
    static Value raise(Env *env, Args &&args);
    static Value Rational(Env *env, Value x, Value y, Value exception);
    static Value Rational(Env *env, Value x, Value y = nullptr, bool exception = true);
    static RationalObject *Rational(Env *env, class Integer x, class Integer y);
    static RationalObject *Rational(Env *env, double arg);
    static Value sleep(Env *env, Value length);
    static Value spawn(Env *, Args &&);
    static Value String(Env *env, Value value);
    static Value test(Env *, Value, Value);
    static Value this_method(Env *env);
    static Value throw_method(Env *, Value, Value = nullptr);

    // instance methods (mixed into Object)

    static bool equal(Value self, Value other) {
        return other == self;
    }

    static Value klass_obj(Env *env, Value self);

    static Value freeze_obj(Env *env, Value self) {
        if (self.is_frozen())
            return self;

        self->freeze();
        return self;
    }

    static Value define_singleton_method(Env *env, Value self, Value name, Block *block);
    static Value dup(Env *env, Value self);
    static Value dup_better(Env *env, Value self); // This will eventually replace `dup`.
    static Value extend(Env *, Value, Args &&);
    static Value hash(Env *env, Value self);
    static Value initialize_copy(Env *env, Value self, Value object);
    static Value inspect(Env *env, Value self);
    static bool instance_variable_defined(Env *env, Value self, Value name_val);
    static Value instance_variable_get(Env *env, Value self, Value name_val);
    static Value instance_variable_set(Env *env, Value self, Value name_val, Value value);
    static Value instance_variables(Env *env, Value self);
    static bool is_a(Env *env, Value self, Value module);
    static bool is_frozen(Value self) { return self.is_frozen(); }
    static Value loop(Env *env, Value self, Block *block);
    static Value method(Env *env, Value self, Value name);
    static Value methods(Env *env, Value self, Value regular_val);
    static bool neqtilde(Env *env, Value self, Value other);
    static Value private_methods(Env *env, Value self, Value recur = nullptr);
    static Value protected_methods(Env *env, Value self, Value recur = nullptr);
    static Value public_methods(Env *env, Value self, Value recur = nullptr);
    static Value remove_instance_variable(Env *env, Value self, Value name_val);
    static bool respond_to_missing(Env *, Value, Value, Value) { return false; }
    static bool respond_to_method(Env *, Value, Value, Value);
    static bool respond_to_method(Env *, Value, Value, bool);
    static Value tap(Env *env, Value self, Block *block);
};

}
