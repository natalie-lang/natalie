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

Vector<Cell *> *Heap::gather_conservative_roots() {
    void *dummy;
    void *top_of_stack = &dummy;
    Vector<Cell *> possible_cells;
    auto roots = new Vector<Cell *>;

    jmp_buf jump_buf;
    setjmp(jump_buf);
    void *raw_jump_buf = reinterpret_cast<void *>(jump_buf);

    for (size_t i = 0; i < sizeof(jump_buf); i += sizeof(intptr_t)) {
        void *ptr = (reinterpret_cast<void **>(raw_jump_buf))[i];
        if (ptr == nullptr)
            continue;
        //printf("registers candidate_cell = %p\n", ptr);
        possible_cells.push(reinterpret_cast<Cell *>(ptr));
    }

    //printf("possible cells = %zu\n", possible_cells.size());

    for (char *stack_address = reinterpret_cast<char *>(top_of_stack); stack_address < m_bottom_of_stack; stack_address += sizeof(intptr_t)) {
        auto candidate_cell = *reinterpret_cast<Cell **>(stack_address);
        //printf("stack candidate_cell = %p\n", candidate_cell);
        possible_cells.push(candidate_cell);
    }

    //printf("possible cells = %zu\n", possible_cells.size());

    for (auto *candidate_cell : possible_cells) {
        auto *candidate_block = HeapBlock::from_cell(candidate_cell);
        //printf("candidate_cell = %p\n", candidate_cell);
        if (is_a_heap_block(candidate_block)) {
            //printf("  is in a heap block\n");
            if (candidate_block->is_cell_in_use(candidate_cell)) {
                //printf("  is in use!\n");
                roots->push(candidate_cell);
            }
        }
    }

    return roots;
}

void Heap::sweep() {
    for (auto *allocator : m_allocators) {
        for (auto *block : *allocator) {
            for (Cell *cell : *block) {
                if (!cell->marked()) {
#ifdef GC_DEBUG
                    char *repr = cell->gc_repr();
                    printf("TRASH: %s\n", repr);
                    delete[] repr;
#endif
                }
            }
        }
    }
}
}
