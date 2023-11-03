#pragma once

#include "natalie/args.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {

class Block : public Cell {
    friend ProcObject;

public:
    enum class BlockType {
        Proc,
        Lambda,
        Method
    };

    Block(Env *env, Value self, MethodFnPtr fn, int arity, BlockType type = BlockType::Proc)
        : m_fn { fn }
        , m_arity { arity }
        , m_env { new Env(*env) }
        , m_self { self }
        , m_type { type } { }

    // NOTE: This should only be called from one of the RUN_BLOCK_* macros!
    Value _run(Env *env, Args args = {}, Block *block = nullptr) {
        assert(has_env());
        Env e { m_env };
        e.set_caller(env);
        e.set_this_block(this);
        args.pop_empty_keyword_hash();
        auto result = m_fn(&e, m_self, args, block);
        return result;
    }

    int arity() const { return m_arity; }

    bool has_env() { return !!m_env; }
    Env *env() { return m_env; }

    void set_type(BlockType type) { m_type = type; }
    bool is_lambda() const { return m_type == BlockType::Lambda; }
    bool is_from_method() const { return m_type == BlockType::Method; }

    Env *calling_env() { return m_calling_env; }
    void set_calling_env(Env *env) { m_calling_env = env; }
    void clear_calling_env() { m_calling_env = nullptr; }

    void set_self(Value self) { m_self = self; }
    Value self() const { return m_self; }

    void copy_fn_pointer_to_method(Method *);

    virtual void visit_children(Visitor &visitor) override final {
        visitor.visit(m_env);
        visitor.visit(m_calling_env);
        visitor.visit(m_self);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Block %p fn=%p>", this, m_fn);
    }

private:
    MethodFnPtr m_fn;
    int m_arity { 0 };
    Env *m_env { nullptr };
    Env *m_calling_env { nullptr };
    Value m_self { nullptr };
    BlockType m_type { BlockType::Proc };
};

}
