#pragma once

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/module_value.hpp"

namespace Natalie {

struct Method : public gc {
    Method(const char *name, ModuleValue *owner, MethodFnPtr fn)
        : m_name { name }
        , m_owner { owner }
        , m_fn { fn }
        , m_undefined { !fn } { }

    Method(const char *name, ModuleValue *owner, Block *block)
        : m_name { name }
        , m_owner { owner }
        , m_env { *block->env() } {
        block->copy_fn_pointer_to_method(this);
        m_env.clear_caller();
    }

    void set_fn(MethodFnPtr fn) { m_fn = fn; }

    Env *env() { return &m_env; }
    bool has_env() { return !!m_env.global_env(); }

    bool is_undefined() { return m_undefined; }

    ValuePtr call(Env *env, ValuePtr self, size_t argc, ValuePtr *args, Block *block) {
        assert(!is_undefined());
        Env *closure_env;
        if (has_env()) {
            closure_env = this->env();
        } else {
            closure_env = m_owner->env();
        }
        Env e = Env { closure_env, env };
        e.set_file(env->file());
        e.set_line(env->line());
        e.set_method_name(m_name);
        e.set_block(block);
        return m_fn(&e, self, argc, args, block);
    }

    const char *name() { return m_name; }
    ModuleValue *owner() { return m_owner; }

private:
    const char *m_name;
    ModuleValue *m_owner;
    MethodFnPtr m_fn;
    Env m_env {};
    bool m_undefined { false };
};

}
