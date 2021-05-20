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
    Heap::the().return_cell_to_free_list(static_cast<Cell *>(ptr));
}

void MarkingVisitor::visit(ValuePtr val) {
    visit(val.value_or_null());
}

Hashmap<Cell *> Heap::gather_conservative_roots() {
    Hashmap<Cell *> roots;

    void *dummy;
    void *end_of_stack = &dummy;

    Vector<Cell *> potential_cells;

    // step over stack, saving potential pointers
    assert(m_start_of_stack);
    assert(end_of_stack);
    if (m_start_of_stack < end_of_stack) {
        for (char *ptr = reinterpret_cast<char *>(m_start_of_stack); ptr < end_of_stack; ptr += sizeof(intptr_t)) {
            Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
            potential_cells.push(potential_cell);
        }
    } else {
        for (char *ptr = reinterpret_cast<char *>(end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
            Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
            potential_cells.push(potential_cell);
        }
    }

    // step over any registers, saving potential pointers
    jmp_buf jump_buf;
    setjmp(jump_buf);
    for (size_t i = 0; i < sizeof(jump_buf); ++i) {
        Cell *potential_cell = reinterpret_cast<Cell **>(jump_buf)[i];
        potential_cells.push(potential_cell);
    }

    // determine which pointers are actual Cells
    for (auto potential_cell : potential_cells) {
        auto potential_block = HeapBlock::from_cell(potential_cell);
        if (is_a_heap_block(potential_block)) {
            auto block = potential_block;
            if (block->is_my_cell_and_in_use(potential_cell)) {
                roots.set(potential_cell);
            }
        }
    }

    return roots;
}

void Heap::collect() {
    if (m_gc_disabled) return;

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
                if (!cell->is_marked()) {
                    //cell->gc_print();
                    //fprintf(stderr, "\n");
                    block->return_cell_to_free_list(cell);
                }
            }
        }
    }
}

}
