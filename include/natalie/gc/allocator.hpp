#pragma once

#include <assert.h>
#include <memory>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/heap_block.hpp"
#include "natalie/macros.hpp"
#include "tm/vector.hpp"

namespace Natalie {

using namespace TM;

class Allocator {
public:
    NAT_MAKE_NONCOPYABLE(Allocator);

    Allocator(size_t cell_size)
        : m_cell_size { cell_size } { }

    ~Allocator() {
        for (auto &block : m_blocks) {
            delete block;
        }
    }

    size_t cell_size() {
        return m_cell_size;
    }

    size_t cell_count_per_block() {
        return (HEAP_BLOCK_SIZE - sizeof(HeapBlock)) / m_cell_size;
    }

    size_t total_cells() {
        return m_blocks.size() * cell_count_per_block();
    }

    size_t free_cells() {
        return m_free_cells;
    }

    short int free_cells_percentage() {
        if (m_blocks.is_empty())
            return 0;
        return free_cells() * 100 / total_cells();
    }

    void *allocate() {
        for (auto block : m_blocks) {
            if (block->has_free()) {
                --m_free_cells;
                return block->find_next_free_cell();
            }
        }
        auto *block = add_heap_block();
        return block->find_next_free_cell();
    }

    bool is_my_block(HeapBlock *candidate_block) {
        for (auto block : m_blocks) {
            if (candidate_block == block)
                return true;
        }
        return false;
    }

    auto begin() {
        return m_blocks.begin();
    }

    auto end() {
        return m_blocks.end();
    }

    void unmark_all_cells_in_all_blocks() {
        for (auto block : *this) {
            block->unmark_all_cells();
        }
    }

private:
    HeapBlock *add_heap_block() {
        auto *block = reinterpret_cast<HeapBlock *>(aligned_alloc(HEAP_BLOCK_SIZE, HEAP_BLOCK_SIZE));
        new (block) HeapBlock(m_cell_size);
        m_blocks.push(block);
        m_free_cells += cell_count_per_block();
        return block;
    }

    size_t m_cell_size;
    size_t m_free_cells { 0 };
    Vector<HeapBlock *> m_blocks;
};

}
