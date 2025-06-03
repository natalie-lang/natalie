#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class ProcObject : public Object {
public:
    static ProcObject *create() {
        return new ProcObject();
    }

    static ProcObject *create(ClassObject *klass) {
        return new ProcObject(klass);
    }

    static ProcObject *create(Block *block, nat_int_t break_point = 0) {
        return new ProcObject(block, break_point);
    }

    static ProcObject *create(const ProcObject &other) {
        return new ProcObject(other);
    }

    static Value from_block_maybe(Block *block) {
        if (!block) {
            return Value::nil();
        }
        return ProcObject::create(block);
    }

    Value initialize(Env *, Block *);

    Block *block() { return m_block; }
    bool is_lambda() { return m_block->is_lambda(); }

    virtual ProcObject *to_proc(Env *) override {
        return this;
    }

    Value call(Env *, Args && = {}, Block * = nullptr);
    bool equal_value(Value) const;
    Value ltlt(Env *, Value);
    Value gtgt(Env *, Value);
    Value ruby2_keywords(Env *);
    Value source_location();
    StringObject *to_s(Env *);

    Env *env() { return m_block->env(); }

    int arity() { return m_block ? m_block->arity() : 0; }

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_block);
    }

    virtual TM::String dbg_inspect(int indent = 0) const override {
        return TM::String::format("<ProcObject {h} block={}>", this, m_block ? m_block->dbg_inspect(indent) : "null");
    }

private:
    ProcObject()
        : Object { Object::Type::Proc, GlobalEnv::the()->Object()->const_fetch("Proc"_s).as_class() } { }

    ProcObject(ClassObject *klass)
        : Object { Object::Type::Proc, klass } { }

    ProcObject(Block *block, nat_int_t break_point = 0)
        : Object { Object::Type::Proc, GlobalEnv::the()->Object()->const_fetch("Proc"_s).as_class() }
        , m_block { block }
        , m_break_point { break_point } {
        assert(m_block);
    }

    ProcObject(const ProcObject &other)
        : Object { other }
        , m_block { other.m_block }
        , m_break_point { other.m_break_point } { }

    Block *m_block { nullptr };
    nat_int_t m_break_point { 0 };
};

}
