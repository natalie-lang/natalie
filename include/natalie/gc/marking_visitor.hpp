#pragma once

#include "natalie/gc/cell.hpp"

namespace Natalie {

class MarkingVisitor : public Cell::Visitor {
public:
    virtual void visit(Cell *cell) override final {
        if (!cell || cell->is_marked()) return;
        cell->mark();
        cell->visit_children(*this);
    }

    virtual void visit(ValuePtr val) override final;
};

}
