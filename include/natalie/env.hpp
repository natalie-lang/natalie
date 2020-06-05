#pragma once

#include <setjmp.h>
#include <stdlib.h>

#include "natalie/forward.hpp"

namespace Natalie {

struct Env {
    Env() { }

    Env(Env *outer)
        : outer { outer } {
        global_env = outer->global_env;
    }

    Env(GlobalEnv *global_env)
        : global_env { global_env } { }

    static Env new_block_env(Env *outer, Env *calling_env) {
        Env env(outer);
        env.block_env = true;
        env.caller = calling_env;
        return env;
    }

    static Env new_detatched_block_env(Env *outer) {
        Env env;
        env.global_env = outer->global_env;
        env.block_env = true;
        return env;
    }

    Value *var_get(const char *, ssize_t);
    Value *var_set(const char *, ssize_t, bool, Value *);

    GlobalEnv *global_env { nullptr };
    Vector *vars { nullptr };
    Env *outer { nullptr };
    Block *block { nullptr };
    bool block_env { false };
    bool rescue { false };
    jmp_buf jump_buf;
    ExceptionValue *exception { nullptr };
    Env *caller { nullptr };
    const char *file { nullptr };
    ssize_t line { 0 };
    const char *method_name { nullptr };
    Value *match { nullptr };
};

}
