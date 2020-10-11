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
        : m_outer { outer } {
        m_global_env = outer->m_global_env;
    }

    Env(GlobalEnv *global_env)
        : m_global_env { global_env } { }

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

    ClassValue *Object() { return m_global_env->Object(); }
    NilValue *nil_obj() { return m_global_env->nil_obj(); }
    TrueValue *true_obj() { return m_global_env->true_obj(); }
    FalseValue *false_obj() { return m_global_env->false_obj(); }

    GlobalEnv *global_env() { return m_global_env; }
    void set_global_env(GlobalEnv *global_env) { m_global_env = global_env; }
    void clear_global_env() { m_global_env = nullptr; }

    void build_vars(ssize_t);

    Env *outer() { return m_outer; }

    Env *caller() { return m_caller; }
    void set_caller(Env *caller) { m_caller = caller; }
    void clear_caller() { m_caller = nullptr; }

    Block *block() { return m_block; }
    void set_block(Block *block) { m_block = block; }

    bool is_block_env() { return m_block_env; }
    void set_block_env() { m_block_env = true; }

    const char *file() { return m_file; }
    void set_file(const char *file) { m_file = file; }

    ssize_t line() { return m_line; }
    void set_line(ssize_t line) { m_line = line; }

    const char *method_name() { return m_method_name; }
    void set_method_name(const char *name) { m_method_name = name; }

    Value *match() { return m_match; }
    void set_match(Value *match) { m_match = match; }
    void clear_match() { m_match = nullptr; }

private:
    GlobalEnv *m_global_env { nullptr };
    Vector<Value *> *m_vars { nullptr };
    Env *m_outer { nullptr };
    Block *m_block { nullptr };
    bool m_block_env { false };
    Env *m_caller { nullptr };
    const char *m_file { nullptr };
    ssize_t m_line { 0 };
    const char *m_method_name { nullptr };
    Value *m_match { nullptr };
};

}
