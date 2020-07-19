#pragma once

#include <stdlib.h>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/vector.hpp"

namespace Natalie {

struct Env : public gc {
    Env() { }

    Env(Env *outer)
        : outer { outer } {
        global_env = outer->global_env;
    }

    Env(GlobalEnv *global_env)
        : global_env { global_env } { }

    static Env new_block_env(Env *, Env *);
    static Env new_detatched_block_env(Env *);

    Value *global_get(const char *);
    Value *global_set(const char *, Value *);

    const char *find_current_method_name();
    char *build_code_location_name(Env *);

    Value *var_get(const char *, ssize_t);
    Value *var_set(const char *, ssize_t, bool, Value *);

    Value *raise(ClassValue *, const char *, ...);
    Value *raise_exception(ExceptionValue *);
    Value *raise_local_jump_error(Value *, const char *);

    Value *last_match();

    ClassValue *Object() { return global_env->Object(); }
    NilValue *nil() { return global_env->nil(); }
    TrueValue *true_obj() { return global_env->true_obj(); }
    FalseValue *false_obj() { return global_env->false_obj(); }

    GlobalEnv *global_env { nullptr };
    Vector<Value *> *vars { nullptr };
    Env *outer { nullptr };
    Block *block { nullptr };
    bool block_env { false };
    Env *caller { nullptr };
    const char *file { nullptr };
    ssize_t line { 0 };
    const char *method_name { nullptr };
    Value *match { nullptr };
};

}
