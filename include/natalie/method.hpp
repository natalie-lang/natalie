#pragma once

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

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

    Value *run(Env *env, Value *self, size_t argc = 0, Value **args = nullptr, Block *block = nullptr) {
        return m_fn(env, self, argc, args, block);
    }

private:
    MethodFnPtr m_fn;
    Env m_env {};
    bool m_undefined { false };
};

}
