#include <stdexcept>

#include "natalie.hpp"

extern "C" void GC_disable() {
    Natalie::Heap::the().gc_disable();
}

namespace Natalie {

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

TM::Hashmap<Cell *> Heap::gather_conservative_roots() {
    Hashmap<Cell *> roots;

    void *dummy;
    void *end_of_stack = &dummy;

    // step over stack, saving potential pointers
    assert(m_start_of_stack);
    assert(end_of_stack);
    assert(m_start_of_stack > end_of_stack);

    for (char *ptr = reinterpret_cast<char *>(end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr); // NOLINT
        if (is_a_heap_cell_in_use(potential_cell)) {
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

    visitor.visit(GlobalEnv::the());
    visitor.visit(NilValue::the());
    visitor.visit(TrueValue::the());
    visitor.visit(FalseValue::the());
    visitor.visit(FiberValue::main());
    visitor.visit(FiberValue::current());

    sweep();
}

void Heap::sweep() {
    for (auto allocator : m_allocators) {
        for (auto block : *allocator) {
            for (auto cell : *block) {
                if (!cell->is_marked() && cell->is_collectible()) {
                    bool had_free = block->has_free();
                    block->return_cell_to_free_list(cell);
                    if (!had_free) {
                        allocator->add_free_block(block);
                    }
                }
            }
        }
    }
}

void *Heap::allocate(size_t size) {
    auto &allocator = find_allocator_of_size(size);

    if (m_gc_enabled) {
#ifdef NAT_GC_DEBUG_ALWAYS_COLLECT
        collect();
#else

        if (allocator.total_cells() == 0) {
            allocator.add_multiple_blocks(initial_blocks_per_allocator);
        } else if (allocator.free_cells_percentage() < min_percent_free_triggers_collection) {
            collect();
            allocator.add_blocks_until_percent_free_reached(min_percent_free_after_collection);
        }
#endif
    }

    return allocator.allocate();
}

void Heap::return_cell_to_free_list(Cell *cell) {
    auto *block = HeapBlock::from_cell(cell);
    assert(is_a_heap_block(block));
    block->return_cell_to_free_list(cell);
}

Cell *HeapBlock::find_next_free_cell() {
    assert(has_free());
    --m_free_count;
    auto node = m_free_list;
    assert(node);
    m_free_list = node->next;
    assert(m_free_list || m_free_count == 0);
    auto cell = reinterpret_cast<Cell *>(node);
    m_used_map[node->index] = true;
    //printf("Cell %p allocated from block %p (size %zu) at index %zu\n", cell, this, m_cell_size, i);
    new (cell) Cell();
    return cell;
}

void HeapBlock::return_cell_to_free_list(const Cell *cell) {
    auto index = index_from_cell(cell);
    assert(index > -1);
    m_used_map[index] = false;
    cell->~Cell();
    memset(&m_memory[index * m_cell_size], 0, m_cell_size);
    auto node = reinterpret_cast<FreeCellNode *>(cell_from_index(index));
    node->next = m_free_list;
    node->index = index;
    m_free_list = node;
    ++m_free_count;
}

}
