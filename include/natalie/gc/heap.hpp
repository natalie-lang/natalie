#pragma once

#include <array>
#include <assert.h>
#include <mutex>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/allocator.hpp"
#include "natalie/gc/marking_visitor.hpp"
#include "natalie/macros.hpp"
#include "tm/hashmap.hpp"
#include "tm/vector.hpp"

namespace Natalie {

extern std::recursive_mutex g_gc_recursive_mutex;

using namespace TM;

class Heap {
public:
    NAT_MAKE_NONCOPYABLE(Heap);

    constexpr static int initial_blocks_per_allocator = 10;
    constexpr static int min_percent_free_triggers_collection = 5;
    constexpr static int min_percent_free_after_collection = 20;

    static Heap &the() {
        if (s_instance)
            return *s_instance;
        s_instance = new Heap();
        return *s_instance;
    }

    ~Heap() {
        for (auto *allocator : m_allocators)
            delete allocator;
    }

    void *allocate(size_t size);

    void collect();

    void return_cell_to_free_list(Cell *cell);

    bool is_a_heap_cell_in_use(Cell *potential_cell) {
        auto potential_block = HeapBlock::from_cell(potential_cell);
        if (is_a_heap_block(potential_block)) {
            auto block = potential_block;
            return block->is_my_cell_and_in_use(potential_cell);
        }
        return false;
    }

    bool gc_enabled() const { return m_gc_enabled; }

    void gc_enable() {
#ifndef NAT_GC_DISABLE
        m_gc_enabled = true;
#endif
    }

    void gc_disable() {
        m_gc_enabled = false;
    }

    size_t total_allocations() const;
    void dump() const;

    template <typename F>
    auto with_gc_disabled(F func) {
        bool was_enabled = m_gc_enabled;
        m_gc_enabled = false;
        auto result = func();
        if (was_enabled)
            m_gc_enabled = true;
        return result;
    }

    bool collect_all_at_exit() const { return m_collect_all_at_exit; }
    void set_collect_all_at_exit(bool collect) { m_collect_all_at_exit = collect; }

private:
    inline static Heap *s_instance = nullptr;

    Heap() {
        m_allocators[0] = new Allocator(64);
        m_allocators[1] = new Allocator(128);
        m_allocators[2] = new Allocator(256);
        m_allocators[3] = new Allocator(512);
        m_allocators[4] = new Allocator(1024);
    }

    std::array<Allocator *, 5> m_allocators;

    Allocator &find_allocator_of_size(size_t size) {
        if (size <= 64)
            return *m_allocators[0];
        else if (size <= 128)
            return *m_allocators[1];
        else if (size <= 256)
            return *m_allocators[2];
        else if (size <= 512)
            return *m_allocators[3];
        else if (size <= 1024)
            return *m_allocators[4];
        fprintf(stderr, "No allocator found for size %zu\n", size);
        NAT_UNREACHABLE();
    }

    void gather_roots_from_asan_fake_stack(Hashmap<Cell *> &, Cell *);
    TM::Hashmap<Cell *> gather_conservative_roots();

    void sweep();

    bool is_a_heap_block(HeapBlock *block) {
        for (auto *allocator : m_allocators) {
            if (allocator->is_my_block(block))
                return true;
        }
        return false;
    }

    bool m_gc_enabled { false };
    bool m_collect_all_at_exit { false };
};

}

extern "C" void GC_disable();
