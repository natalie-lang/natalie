#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/allocator.hpp"
#include "natalie/gc/marking_visitor.hpp"
#include "natalie/macros.hpp"
#include "tm/vector.hpp"

namespace Natalie {

using namespace TM;

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
        }
#endif
        return allocator.allocate();
    }

    void collect() {
#ifdef NAT_GC_DISABLE
        return;
#endif
        // TODO
    }

    void *bottom_of_stack() {
        return m_bottom_of_stack;
    }

    void set_bottom_of_stack(void *bottom_of_stack) {
        m_bottom_of_stack = bottom_of_stack;
    }

    void add_cell_to_free_list(Cell *cell) {
        auto *block = HeapBlock::from_cell(cell);
        assert(is_a_heap_block(block));
        block->add_cell_to_free_list(cell);
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

    Vector<Cell *> gather_conservative_roots();

    void sweep();

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
