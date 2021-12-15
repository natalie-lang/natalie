#pragma once

#include "natalie/gc/cell.hpp"
#include "tm/vector.hpp"

namespace Natalie {

class MarkingVisitor : public Cell::Visitor {
public:
    virtual void visit(Cell *cell) override final {
        if (!cell || cell->is_marked()) return;
        cell->mark();
        cell->visit_children(*this);
    }

    virtual void visit(const Cell *cell) override final {
        visit(const_cast<Cell *>(cell));
    }

    virtual void visit(Value val) override final;
};

}
