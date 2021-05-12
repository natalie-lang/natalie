#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/allocator.hpp"
#include "natalie/gc/marking_visitor.hpp"
#include "natalie/macros.hpp"
#include "natalie/vector.hpp"

namespace Natalie {

class Heap {
public:
    NAT_MAKE_NONCOPYABLE(Heap);

    static Heap &the() {
        if (s_instance)
            return *s_instance;
        s_instance = new Heap();
        return *s_instance;
    }

    void *allocate(size_t size) {
        auto &allocator = find_allocator_of_size(size);
#ifndef NAT_GC_DISABLE
        auto free = allocator.free_cells_percentage();
        if (free > 0 && free < 10) {
            //std::cerr << free << "% free in allocator of cell size " << size << "\n";
            //collect();
        }
#endif
        return allocator.allocate();
    }

    void collect() {
#ifdef NAT_GC_DISABLE
        return;
#endif
        MarkingVisitor visitor;
        auto roots = gather_conservative_roots();
        for (auto cell : roots) {
            cell->visit_children(visitor);
        }
        sweep();
    }

    void *bottom_of_stack() {
        return m_bottom_of_stack;
    }

    void set_bottom_of_stack(void *bottom_of_stack) {
        m_bottom_of_stack = bottom_of_stack;
    }

    void free_cell(Cell *cell) {
        auto *block = HeapBlock::from_cell(cell);
        assert(is_a_heap_block(block));
        block->free_cell(cell);
    }

private:
    static Heap *s_instance;

    Heap() {
        m_allocators.push(new Allocator(16));
        m_allocators.push(new Allocator(32));
        m_allocators.push(new Allocator(64));
        m_allocators.push(new Allocator(128));
        m_allocators.push(new Allocator(256));
        m_allocators.push(new Allocator(512));
        m_allocators.push(new Allocator(1024));
    }

    ~Heap() {
        for (auto *allocator : m_allocators) {
            delete allocator;
        }
    }

    Vector<Cell *> gather_conservative_roots() {
        void *dummy;
        void *top_of_stack = &dummy;
        Vector<Cell *> possible_cells;
        Vector<Cell *> roots;

        jmp_buf jump_buf;
        setjmp(jump_buf);
        void *raw_jump_buf = reinterpret_cast<void *>(jump_buf);

        for (size_t i = 0; i < ((size_t)sizeof(jump_buf)) / sizeof(intptr_t); i += sizeof(intptr_t)) {
            void *offset = (reinterpret_cast<void **>(raw_jump_buf))[i];
            if (offset == nullptr)
                continue;
            possible_cells.push(reinterpret_cast<Cell *>(offset));
        }

        for (char *stack_address = reinterpret_cast<char *>(top_of_stack); stack_address < m_bottom_of_stack; stack_address += sizeof(intptr_t)) {
            possible_cells.push(reinterpret_cast<Cell *>(stack_address));
        }

        for (auto *candidate_cell : possible_cells) {
            auto *candidate_block = HeapBlock::from_cell(candidate_cell);
            if (is_a_heap_block(candidate_block) && candidate_block->cell_in_use(candidate_cell))
                roots.push(candidate_cell);
        }

        return roots;
    }

    void sweep() {
        for (auto *allocator : m_allocators) {
            for (auto block : *allocator) {
            }
        }
    }

    Allocator &find_allocator_of_size(size_t size) {
        for (auto *allocator : m_allocators) {
            if (allocator->cell_size() >= size) {
                return *allocator;
            }
        }
        abort();
    }

    bool is_a_heap_block(HeapBlock *block) {
        for (auto *allocator : m_allocators) {
            if (allocator->is_my_block(block))
                return true;
        }
        return false;
    }

    Vector<Allocator *> m_allocators;

    void *m_bottom_of_stack { nullptr };
};

}
