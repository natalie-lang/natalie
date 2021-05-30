#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/allocator.hpp"
#include "natalie/gc/marking_visitor.hpp"
#include "natalie/hashmap.hpp"
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

    void *allocate(size_t size);

    void collect();

    void *start_of_stack() {
        return m_start_of_stack;
    }

    void set_start_of_stack(void *start_of_stack) {
        assert(start_of_stack);
        m_start_of_stack = start_of_stack;
    }

    void return_cell_to_free_list(Cell *cell) {
        auto *block = HeapBlock::from_cell(cell);
        assert(is_a_heap_block(block));
        block->return_cell_to_free_list(cell);
    }

    bool is_a_heap_cell_in_use(Cell *potential_cell) {
        auto potential_block = HeapBlock::from_cell(potential_cell);
        if (is_a_heap_block(potential_block)) {
            auto block = potential_block;
            return block->is_my_cell_and_in_use(potential_cell);
        }
        return false;
    }

    bool gc_enabled() { return m_gc_enabled; }

    void gc_enable() {
        m_gc_enabled = true;
    }

    void gc_disable() {
        m_gc_enabled = false;
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

    Hashmap<Cell *> gather_conservative_roots();

    void sweep();

    Allocator &find_allocator_of_size(size_t size) {
        for (auto *allocator : m_allocators) {
            if (allocator->cell_size() >= size) {
                return *allocator;
            }
        }
        NAT_UNREACHABLE();
    }

    bool is_a_heap_block(HeapBlock *block) {
        for (auto *allocator : m_allocators) {
            if (allocator->is_my_block(block))
                return true;
        }
        return false;
    }

    Vector<Allocator *> m_allocators;

    void *m_start_of_stack { nullptr };
    bool m_gc_enabled { false };
};

}

extern "C" void GC_disable();
