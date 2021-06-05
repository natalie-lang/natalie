#pragma once

#include "natalie/gc/cell.hpp"
#include "tm/vector.hpp"

namespace Natalie {

class MarkingVisitor : public Cell::Visitor {
public:
    virtual void visit(Cell *cell) override final {
        if (!cell || cell->is_marked()) return;
#ifdef NAT_GC_FIND_BUGS
        m_chain.push(cell);
        if (cell->m_collected) {
            cell->gc_print();
            fprintf(stderr, " was already collected!\nHow we got here:\n");
            int indent = 1;
            for (auto *link : m_chain) {
                for (int i = 0; i < indent; ++i)
                    fprintf(stderr, "  ");
                link->gc_print();
                fprintf(stderr, "\n");
                ++indent;
            }
            abort();
        }
#endif
        cell->mark();
        cell->visit_children(*this);
#ifdef NAT_GC_FIND_BUGS
        m_chain.pop();
#endif
    }

    virtual void visit(const Cell *cell) override final {
        visit(const_cast<Cell *>(cell));
    }

    virtual void visit(ValuePtr val) override final;

private:
#ifdef NAT_GC_FIND_BUGS
    TM::Vector<Cell *> m_chain {};
#endif
};

}
