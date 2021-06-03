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
        , m_env { env }
        , m_self { self } {
        m_env->clear_caller();
    }

    // NOTE: This should only be called from one of the RUN_BLOCK_* macros!
    ValuePtr _run(Env *env, size_t argc = 0, ValuePtr *args = nullptr, Block *block = nullptr) {
        assert(has_env());
        auto *e = new Env { m_env };
        e->set_caller(env);
        auto result = m_fn(e, m_self, argc, args, block);
        e->clear_caller();
        return result;
    }

    int arity() { return m_arity; }

    bool has_env() { return !!m_env; }
    Env *env() { return m_env; }

    void set_self(ValuePtr self) { m_self = self; }

    void copy_fn_pointer_to_method(Method *);

    virtual void visit_children(Visitor &visitor) override final {
        visitor.visit(m_self);
        visitor.visit(m_env);
    }

    virtual void gc_print() override {
        fprintf(stderr, "<Block %p fn=%p>", this, m_fn);
    }

private:
    MethodFnPtr m_fn;
    int m_arity { 0 };
    Env *m_env { nullptr };
    ValuePtr m_self { nullptr };
};

}
