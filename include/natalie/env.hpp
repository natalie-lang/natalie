#pragma once

#include <stdlib.h>

#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/global_env.hpp"
#include "natalie/value_ptr.hpp"
#include "natalie/vector.hpp"

namespace Natalie {

struct Env : public gc {
    Env() { }

    Env(Env *outer)
        : m_outer { outer } {
        m_global_env = outer->m_global_env;
    }

    Env(Env *outer, Env *calling_env)
        : m_outer { outer }
        , m_caller { calling_env } {
        m_global_env = outer->m_global_env;
    }

    Env(GlobalEnv *global_env)
        : m_global_env { global_env } { }

    static Env new_detatched_env(Env *);

    ValuePtr global_get(SymbolValue *);
    ValuePtr global_set(SymbolValue *, ValuePtr);

    const char *find_current_method_name();
    char *build_code_location_name(Env *);

    ValuePtr var_get(const char *, size_t);
    ValuePtr var_set(const char *, size_t, bool, ValuePtr);

    [[noreturn]] void raise(ClassValue *, StringValue *);
    [[noreturn]] void raise(ClassValue *, const char *, ...);
    [[noreturn]] void raise(const char *, const char *, ...);
    [[noreturn]] void raise_exception(ExceptionValue *);
    [[noreturn]] void raise_local_jump_error(ValuePtr, const char *);
    [[noreturn]] void raise_errno();

    void assert_argc(size_t argc, size_t expected);
    void assert_argc(size_t argc, size_t expected_low, size_t expected_high);
    void assert_argc_at_least(size_t argc, size_t expected);
    void assert_block_given(Block *);

    ValuePtr last_match();

    ClassValue *Array() { return m_global_env->Array(); }
    ClassValue *Class() { return m_global_env->Class(); }
    ClassValue *Hash() { return m_global_env->Hash(); }
    ClassValue *Integer() { return m_global_env->Integer(); }
    ClassValue *Module() { return m_global_env->Module(); }
    ClassValue *Object() { return m_global_env->Object(); }
    ClassValue *Regexp() { return m_global_env->Regexp(); }
    ClassValue *String() { return m_global_env->String(); }
    ClassValue *Symbol() { return m_global_env->Symbol(); }
    NilValue *nil_obj() { return m_global_env->nil_obj(); }
    TrueValue *true_obj() { return m_global_env->true_obj(); }
    FalseValue *false_obj() { return m_global_env->false_obj(); }

    GlobalEnv *global_env() { return m_global_env; }
    void set_global_env(GlobalEnv *global_env) { m_global_env = global_env; }
    void clear_global_env() { m_global_env = nullptr; }

    void build_vars(size_t);

    Env *outer() { return m_outer; }

    Env *caller() { return m_caller; }
    void set_caller(Env *caller) { m_caller = caller; }
    void clear_caller() { m_caller = nullptr; }

    Block *block() { return m_block; }
    void set_block(Block *block) { m_block = block; }

    const char *file() { return m_file; }
    void set_file(const char *file) { m_file = file; }

    size_t line() { return m_line; }
    void set_line(size_t line) { m_line = line; }

    const char *method_name() { return m_method_name; }
    void set_method_name(const char *name) { m_method_name = name; }

    ValuePtr match() { return m_match; }
    void set_match(ValuePtr match) { m_match = match; }
    void clear_match() { m_match = nullptr; }

private:
    GlobalEnv *m_global_env { nullptr };
    Vector<ValuePtr> *m_vars { nullptr };
    Env *m_outer { nullptr };
    Block *m_block { nullptr };
    Env *m_caller { nullptr };
    const char *m_file { nullptr };
    size_t m_line { 0 };
    const char *m_method_name { nullptr };
    ValuePtr m_match { nullptr };
};

}
