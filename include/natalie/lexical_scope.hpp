#pragma once

#include "natalie/gc.hpp"

namespace Natalie {

class LexicalScope : public Cell {
public:
    LexicalScope(LexicalScope *parent, ModuleObject *module)
        : m_parent(parent)
        , m_module(module) { }

    LexicalScope *parent() const { return m_parent; }
    ModuleObject *module() const { return m_module; }

    virtual void visit_children(Visitor &visitor) const override final;

private:
    LexicalScope *m_parent { nullptr };
    ModuleObject *m_module { nullptr };
};
}
