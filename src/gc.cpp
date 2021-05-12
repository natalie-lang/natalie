#include "natalie.hpp"

namespace Natalie {
Heap *Heap::s_instance = nullptr;

void *Cell::operator new(size_t size) {
    auto *cell = Heap::the().allocate(size);
    assert(cell);
    return cell;
}

void Cell::operator delete(void *ptr) {
    Heap::the().free_cell(static_cast<Cell *>(ptr));
}

void MarkingVisitor::visit(ValuePtr val) {
    visit(val.value_or_null());
}
}
