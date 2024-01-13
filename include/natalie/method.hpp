#pragma once

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/method_visibility.hpp"
#include "natalie/module_object.hpp"

namespace Natalie {

class Method : public gc {
public:
    Method(TM::String &&name, ModuleObject *owner, MethodFnPtr fn, int arity)
        : m_name { std::move(name) }
        , m_owner { owner }
        , m_fn { fn }
        , m_arity { arity } {
        assert(fn);
    }

    Method(TM::String &&name, ModuleObject *owner, MethodFnPtr fn, int arity, const String &file, const size_t line)
        : m_name { std::move(name) }
        , m_owner { owner }
        , m_fn { fn }
        , m_arity { arity }
        , m_file { file }
        , m_line { line } {
        assert(fn);
    }

    Method(TM::String &&name, ModuleObject *owner, Block *block)
        : m_name { std::move(name) }
        , m_owner { owner }
        , m_arity { block->arity() }
        , m_env { new Env(*block->env()) } {
        assert(m_env);
        block->copy_fn_pointer_to_method(this);

        if (block->is_from_method())
            m_self = block->self();

        if (block->env()->file()) {
            m_file = block->env()->file();
            m_line = block->env()->line();
        }
    }

    Method(const TM::String &name, ModuleObject *owner, MethodFnPtr fn, int arity)
        : Method(TM::String(name), owner, fn, arity) { }
    Method(const TM::String &name, ModuleObject *owner, MethodFnPtr fn, int arity, const String &file, const size_t line)
        : Method(TM::String(name), owner, fn, arity, file, line) { }
    Method(const TM::String &name, ModuleObject *owner, Block *block)
        : Method(TM::String(name), owner, block) { }

    MethodFnPtr fn() { return m_fn; }
    void set_fn(MethodFnPtr fn) { m_fn = fn; }

    bool has_env() const { return !!m_env; }
    Env *env() const { return m_env; }

    bool is_optimized() const { return m_optimized; }
    void set_optimized(bool optimized) { m_optimized = optimized; }

    Value call(Env *env, Value self, Args args, Block *block) const;

    String name() const { return m_name; }
    ModuleObject *owner() const { return m_owner; }

    int arity() const { return m_arity; }

    const Optional<String> &get_file() const { return m_file; }
    const Optional<size_t> &get_line() const { return m_line; }

    virtual void gc_inspect(char *buf, size_t len) const {
        snprintf(buf, len, "<Method %p name='%s' fn=%p>", this, m_name.c_str(), m_fn);
    }

private:
    String m_name {};
    ModuleObject *m_owner;
    MethodFnPtr m_fn;
    Value m_self { nullptr };
    int m_arity { 0 };
    Optional<String> m_file {};
    Optional<size_t> m_line {};
    Env *m_env { nullptr };
    bool m_optimized { false };
};
}
