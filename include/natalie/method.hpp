#pragma once

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/module_value.hpp"

namespace Natalie {

struct Method : public gc {
    Method(MethodFnPtr fn)
        : m_fn { fn }
        , m_undefined { !fn } { }

    Method(Block *block)
        : m_env { *block->env() } {
        block->copy_fn_pointer_to_method(this);
        m_env.clear_caller();
    }

    void set_fn(MethodFnPtr fn) { m_fn = fn; }

    Env *env() { return &m_env; }
    bool has_env() { return !!m_env.global_env(); }

    bool is_undefined() { return m_undefined; }

    ValuePtr run(Env *env, ValuePtr self, size_t argc = 0, ValuePtr *args = nullptr, Block *block = nullptr) {
        return m_fn(env, self, argc, args, block);
    }

    ValuePtr call(Env *env, ModuleValue *method_owner, const char *method_name, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
        assert(!is_undefined());
        Env *closure_env;
        if (has_env()) {
            closure_env = this->env();
        } else {
            closure_env = method_owner->env();
        }
        Env e = Env { closure_env, env };
        e.set_file(env->file());
        e.set_line(env->line());
        e.set_method_name(method_name);
        e.set_block(block);
        return run(&e, self, argc, args, block);
    }

private:
    MethodFnPtr m_fn;
    Env m_env {};
    bool m_undefined { false };
};

}
