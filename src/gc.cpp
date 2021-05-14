#include "natalie.hpp"

namespace Natalie {

Heap *Heap::s_instance = nullptr;

void *Cell::operator new(size_t size) {
    auto *cell = Heap::the().allocate(size);
    assert(cell);
    return cell;
}

void Cell::operator delete(void *ptr) {
    Heap::the().add_cell_to_free_list(static_cast<Cell *>(ptr));
}

void MarkingVisitor::visit(ValuePtr val) {
    visit(val.value_or_null());
}

Vector<Cell *> Heap::gather_conservative_roots() {
    auto roots = Vector<Cell *> {};
    // TODO
    return roots;
}

void Heap::sweep() {
    // TODO
}

}
