#pragma once

#include "natalie/gc/cell.hpp"
#include "tm/vector.hpp"
#include <stack>

namespace Natalie {

class MarkingVisitor : public Cell::Visitor {
public:
    virtual void visit(Value val) override final;

    virtual void visit(const Cell *cell) override final {
        if (!cell || cell->is_visited()) return;
        m_stack.push(cell);
    }

    void visit_all() {
        while (!m_stack.empty()) {
            auto cell = m_stack.top();
            m_stack.pop();
            cell->mark();
            cell->visit_children(*this);
        }
    }

private:
    std::stack<const Cell *> m_stack;
};

}
