#include "natalie.hpp"

extern "C" void GC_disable() {
    Natalie::Heap::the().gc_disable();
}

namespace Natalie {

Heap *Heap::s_instance = nullptr;

void *Cell::operator new(size_t size) {
    auto *cell = Heap::the().allocate(size);
    assert(cell);
    return cell;
}

void Cell::operator delete(void *ptr) {
    // We may need this in the future, but for now, let's just get a big
    // beautiful abort if anything is trying to delete a GC-allocated object.
    NAT_UNREACHABLE();
}

void MarkingVisitor::visit(ValuePtr val) {
    visit(val.value_or_null());
}

Hashmap<Cell *> Heap::gather_conservative_roots() {
    Hashmap<Cell *> roots;

    void *dummy;
    void *end_of_stack = &dummy;

    // step over stack, saving potential pointers
    assert(m_start_of_stack);
    assert(end_of_stack);
    if (m_start_of_stack < end_of_stack) {
        for (char *ptr = reinterpret_cast<char *>(m_start_of_stack); ptr < end_of_stack; ptr += sizeof(intptr_t)) {
            Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
            if (is_a_heap_cell_in_use(potential_cell))
                roots.set(potential_cell);
        }
    } else {
        for (char *ptr = reinterpret_cast<char *>(end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
            Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
            if (is_a_heap_cell_in_use(potential_cell))
                roots.set(potential_cell);
        }
    }

    // step over any registers, saving potential pointers
    jmp_buf jump_buf;
    setjmp(jump_buf);
    for (char *i = (char *)jump_buf; i < (char *)jump_buf + sizeof(jump_buf); ++i) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(i);
        if (is_a_heap_cell_in_use(potential_cell)) {
            roots.set(potential_cell);
        }
    }

    return roots;
}

void Heap::collect() {
    if (!m_gc_enabled) return;

    for (auto allocator : m_allocators) {
        allocator->unmark_all_cells_in_all_blocks();
    }

    MarkingVisitor visitor;

    auto roots = gather_conservative_roots();
    for (auto pair : roots) {
        visitor.visit(pair.first);
    }

    sweep();
}

void Heap::sweep() {
    for (auto allocator : m_allocators) {
        for (auto block : *allocator) {
            for (auto cell : *block) {
                if (!cell->is_marked() && cell->is_collectible()) {
                    //cell->gc_print();
                    //fprintf(stderr, "\n");
                    block->return_cell_to_free_list(cell);
                }
            }
        }
    }
}

void *Heap::allocate(size_t size) {
    auto &allocator = find_allocator_of_size(size);

    if (m_gc_enabled) {
        //collect();
    }

    return allocator.allocate();
}

}
