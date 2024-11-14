#pragma once

#include "natalie/gc/cell.hpp"
#include "tm/vector.hpp"
#include <stack>

// time boardslam.rb 3 5 1
// 1.656 using C stack
// 1.692 using std::stack
// 2.847 using std::queue

namespace Natalie {

class MarkingVisitor : public Cell::Visitor {
public:
    virtual void visit(Cell *cell) override final {
        if (!cell || cell->is_marked()) return;
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
    std::stack<Cell *> m_stack;
};

}
