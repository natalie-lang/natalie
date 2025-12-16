#pragma once

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/module_object.hpp"

namespace Natalie {

class Method : public Cell {
public:
    Method(TM::String &&name, ModuleObject *owner, MethodFnPtr fn, int arity, int break_point = 0, const char *file = nullptr, size_t line = 0)
        : m_name { std::move(name) }
        , m_owner { owner }
        , m_fn { fn }
        , m_arity { arity }
        , m_break_point { break_point }
        , m_file { file ? Optional<String>(file) : Optional<String>() }
        , m_line { line ? Optional<size_t>(line) : Optional<size_t>() } {
        assert(fn);
    }

    Method(TM::String &&name, ModuleObject *owner, Block *block)
        : m_name { std::move(name) }
        , m_owner { owner }
        , m_arity { block->arity() }
        , m_env { Env::create(*block->env()) } {
        assert(m_env);
        block->copy_fn_pointer_to_method(this);

        if (block->has_return())
            m_break_point = -1;

        if (block->is_from_method())
            m_self = block->self();

        if (block->env()->file()) {
            m_file = block->env()->file();
            m_line = block->env()->line();
        }
    }

    Method(const TM::String &name, ModuleObject *owner, MethodFnPtr fn, int arity, int break_point = 0, const char *file = nullptr, size_t line = 0)
        : Method(TM::String(name), owner, fn, arity, break_point, file, line) { }

    Method(const TM::String &name, ModuleObject *owner, Block *block)
        : Method(TM::String(name), owner, block) { }

    static Method *from_other(const TM::String &name, ModuleObject *owner, Method *other) {
        auto method = new Method { name, owner, other->fn(), other->arity() };
        method->m_self = other->m_self;
        method->m_env = other->m_env;
        method->m_file = other->m_file;
        method->m_line = other->m_line;
        method->m_break_point = other->m_break_point;
        method->set_original_method(other);
        return method;
    }

    MethodFnPtr fn() { return m_fn; }
    void set_fn(MethodFnPtr fn) { m_fn = fn; }

    bool has_env() const { return !!m_env; }
    Env *env() const { return m_env; }

    Value call(Env *env, Value self, Args &&args, Block *block) const;

    String name() const { return m_name; }
    ModuleObject *owner() const { return m_owner; }

    String original_name() const {
        if (m_original_method)
            return m_original_method->name();
        return name();
    }
    void set_original_method(Method *method) { m_original_method = method; }
    Method *original_method() const { return m_original_method; }

    int arity() const { return m_arity; }

    const Optional<String> &get_file() const { return m_file; }
    const Optional<size_t> &get_line() const { return m_line; }

    virtual void visit_children(Visitor &visitor) const override final {
        visitor.visit(m_owner);
        visitor.visit(m_env);
        if (m_self)
            visitor.visit(m_self.value());
        visitor.visit(m_original_method);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<Method {h} name=\"{}\" fn={h}>", this, m_name, reinterpret_cast<void *>(m_fn));
    }

private:
    String m_name {};
    Method *m_original_method = nullptr;
    ModuleObject *m_owner;
    MethodFnPtr m_fn;
    Optional<Value> m_self {};
    int m_arity { 0 };
    int m_break_point { 0 };
    Optional<String> m_file {};
    Optional<size_t> m_line {};
    Env *m_env { nullptr };
};
}
