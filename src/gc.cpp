#include "natalie.hpp"

namespace Natalie {
Heap *Heap::s_instance = nullptr;

void *Cell::operator new(size_t size) {
    auto *cell = Heap::the().allocate(size);
    assert(cell);
    return cell;
}

void MarkingVisitor::visit(ValuePtr val) {
    visit(val.value_or_null());
}
}
