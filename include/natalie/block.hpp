#pragma once

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {

struct Block : public gc {

    Block(Env &env, Value *self, MethodFnPtr fn)
        : m_fn { fn }
        , m_env { env }
        , m_self { self } {
        m_env.clear_caller();
    }

    // NOTE: This should only be called from one of the RUN_BLOCK_* macros!
    Value *_run(Env *env, ssize_t argc = 0, Value **args = nullptr, Block *block = nullptr) {
        Env e = Env { &m_env, env };
        return m_fn(&e, m_self, argc, args, block);
    }

    Env *env() { return &m_env; }

    void set_self(Value *self) { m_self = self; }

    void copy_fn_pointer_to_method(Method *);

private:
    MethodFnPtr m_fn;
    Env m_env {};
    Value *m_self { nullptr };
};

}
