#pragma once

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {

class Block : public Cell {
    friend ProcValue;

public:
    Block(Env *env, ValuePtr self, MethodFnPtr fn, int arity)
        : m_fn { fn }
        , m_arity { arity }
        , m_env { new Env(*env) }
        , m_self { self } { }

    // NOTE: This should only be called from one of the RUN_BLOCK_* macros!
    ValuePtr _run(Env *env, size_t argc = 0, ValuePtr *args = nullptr, Block *block = nullptr) {
        assert(has_env());
        Env e { m_env };
        e.set_caller(env);
        e.set_this_block(this);
        auto result = m_fn(&e, m_self, argc, args, block);
        return result;
    }

    int arity() { return m_arity; }

    bool has_env() { return !!m_env; }
    Env *env() { return m_env; }

    Env *calling_env() { return m_calling_env; }
    void set_calling_env(Env *env) { m_calling_env = env; }
    void clear_calling_env() { m_calling_env = nullptr; }

    void set_self(ValuePtr self) { m_self = self; }

    void copy_fn_pointer_to_method(Method *);

    virtual void visit_children(Visitor &visitor) override final {
        visitor.visit(m_env);
        //visitor.visit(m_calling_env); memory access bug :-(
        visitor.visit(m_self);
    }

    virtual void gc_inspect(char *buf, size_t len) override {
        snprintf(buf, len, "<Block %p fn=%p>", this, m_fn);
    }

private:
    MethodFnPtr m_fn;
    int m_arity { 0 };
    Env *m_env { nullptr };
    Env *m_calling_env { nullptr };
    ValuePtr m_self { nullptr };
};

}
