#pragma once

#include "natalie/args.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "tm/owned_ptr.hpp"

namespace Natalie {

class Block : public Cell {
    friend ProcObject;

public:
    enum class BlockType {
        Proc,
        Lambda,
        Method
    };

    Block(Env &env, Value self, MethodFnPtr fn, int arity, BlockType type = BlockType::Proc)
        : m_fn { fn }
        , m_arity { arity }
        , m_env { new Env(env) }
        , m_self { self }
        , m_type { type } { }

    // Keep the TM:: namespace, ffi-clang (used in gc_lint) gets confused otherwise
    Block(TM::OwnedPtr<Env> &&env, Value self, MethodFnPtr fn, int arity, BlockType type = BlockType::Proc)
        : m_fn { fn }
        , m_arity { arity }
        , m_env { env.release() }
        , m_self { self }
        , m_type { type } { }

    Value run(Env *env, Args &&args = {}, Block *block = nullptr);

    int arity() const { return m_arity; }

    Env *env() { return m_env; }

    void set_type(BlockType type) { m_type = type; }
    bool is_lambda() const { return m_type == BlockType::Lambda; }
    bool is_from_method() const { return m_type == BlockType::Method; }

    Env *calling_env() { return m_calling_env; }
    void set_calling_env(Env *env) { m_calling_env = env; }
    void clear_calling_env() { m_calling_env = nullptr; }

    void set_self(Value self) {
        m_self = self;
    }
    Value self() const { return m_self; }

    void copy_fn_pointer_to_method(Method *);

    virtual void visit_children(Visitor &visitor) const override final {
        visitor.visit(m_env);
        visitor.visit(m_calling_env);
        visitor.visit(m_self);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<Block {h} fn={h}>", this, reinterpret_cast<void *>(m_fn));
    }

private:
    MethodFnPtr m_fn;
    int m_arity { 0 };
    Env *m_env { nullptr };
    Env *m_calling_env { nullptr };
    Value m_self {};
    BlockType m_type { BlockType::Proc };
};

}
