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
    Method(const char *name, ModuleObject *owner, MethodFnPtr fn, int arity, MethodVisibility visibility)
        : Method { name, name, owner, fn, arity, visibility } { }

    Method(const char *name, const char *original_name, ModuleObject *owner, MethodFnPtr fn, int arity, MethodVisibility visibility)
        : m_name { name }
        , m_original_name { original_name }
        , m_owner { owner }
        , m_fn { fn }
        , m_arity { arity }
        , m_undefined { !fn }
        , m_visibility { visibility } { }

    Method(const char *name, ModuleObject *owner, Block *block, MethodVisibility visibility)
        : Method { name, name, owner, block, visibility } { }

    Method(const char *name, const char *original_name, ModuleObject *owner, Block *block, MethodVisibility visibility)
        : m_name { name }
        , m_original_name { original_name }
        , m_owner { owner }
        , m_arity { block->arity() }
        , m_env { new Env(*block->env()) }
        , m_visibility { visibility } {
        block->copy_fn_pointer_to_method(this);
        assert(m_env);
    }

    Method *create_alias(const char *new_name) {
        Method *method = new Method { new_name, m_name.c_str(), m_owner, m_fn, m_arity, m_visibility };
        method->m_env = m_env;
        return method;
    }

    MethodFnPtr fn() { return m_fn; }
    void set_fn(MethodFnPtr fn) { m_fn = fn; }

    bool has_env() { return !!m_env; }
    Env *env() { return m_env; }

    bool is_undefined() const { return m_undefined; }

    Value call(Env *env, Value self, size_t argc, Value *args, Block *block) {
        assert(!is_undefined());
        Env *closure_env;
        if (has_env()) {
            closure_env = this->env();
        } else {
            closure_env = m_owner->env();
        }
        Env e { closure_env };
        e.set_caller(env);
        e.set_method(this);
        e.set_file(env->file());
        e.set_line(env->line());
        e.set_block(block);
        Value result;
        if (block && !block->calling_env()) {
            block->set_calling_env(env);
            result = m_fn(&e, self, argc, args, block);
            block->clear_calling_env();
        } else {
            result = m_fn(&e, self, argc, args, block);
        }
        return result;
    }

    const String *name() { return &m_name; }
    const String *original_name() { return &m_original_name; }
    ModuleObject *owner() { return m_owner; }

    MethodVisibility visibility() { return m_visibility; }
    void set_visibility(MethodVisibility visibility) { m_visibility = visibility; }

    int arity() const { return m_arity; }

    virtual void visit_children(Visitor &visitor) override final {
        visitor.visit(m_owner);
        visitor.visit(m_env);
    }

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<Method %p name='%s' fn=%p>", this, m_name.c_str(), m_fn);
    }

private:
    String m_name {};
    String m_original_name {};
    ModuleObject *m_owner;
    MethodFnPtr m_fn;
    int m_arity { 0 };
    Env *m_env { nullptr };
    bool m_undefined { false };
    MethodVisibility m_visibility { MethodVisibility::Public };
};
}
