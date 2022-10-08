#pragma once

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "natalie/gc/cell.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

constexpr const size_t HEAP_BLOCK_SIZE = 32 * 1024;
constexpr const size_t HEAP_BLOCK_MASK = ~(HEAP_BLOCK_SIZE - 1);
constexpr const size_t HEAP_CELL_COUNT_MAX = HEAP_BLOCK_SIZE / 16; // 16 bytes is the smallest cell we will allocate

class HeapBlock {
public:
    struct FreeCellNode {
        FreeCellNode *next;
        size_t index;
    };

    NAT_MAKE_NONCOPYABLE(HeapBlock);

    HeapBlock(size_t size)
        : m_cell_size { size }
        , m_total_count { (HEAP_BLOCK_SIZE - sizeof(HeapBlock)) / m_cell_size }
        , m_free_count { m_total_count } {
        memset(m_memory, 0, HEAP_BLOCK_SIZE - sizeof(HeapBlock));
        for (int i = m_total_count - 1; i >= 0; --i) {
            auto node = reinterpret_cast<FreeCellNode *>(cell_from_index(i));
            node->next = m_free_list;
            node->index = i;
            m_free_list = node;
        }
    }

    ~HeapBlock() {
        for (auto cell : *this) {
            return_cell_to_free_list(cell);
        }
    }

    // returns a pointer to a *potential* block (you still have to check it!)
    static HeapBlock *from_cell(const Cell *cell) {
        return reinterpret_cast<HeapBlock *>((intptr_t)cell & HEAP_BLOCK_MASK);
    }

    Cell *cell_from_index(size_t index) {
        void *cell = &m_memory[index * m_cell_size];
        return static_cast<Cell *>(cell);
    }

    // returns true if the supposed Cell* belongs to this block and it's in use
    bool is_my_cell_and_in_use(const Cell *cell) const {
        auto index = index_from_cell(cell);
        if (index == -1)
            return false;
        return m_used_map[index];
    }

    bool has_free() const {
        return m_free_count > 0;
    }

    Cell *find_next_free_cell();

    void return_cell_to_free_list(const Cell *cell);

    size_t total_count() const { return m_total_count; }

    size_t used_count() const { return m_total_count - m_free_count; }

    class iterator {
    public:
        iterator(HeapBlock *block, size_t index)
            : m_block { block }
            , m_index { index } {
            m_ptr = m_block->cell_from_index(m_index);
        }

        iterator operator++() {
            m_index = m_block->next_used_index_from(m_index + 1);
            m_ptr = m_block->cell_from_index(m_index);
            return *this;
        }

        iterator operator++(int _) {
            iterator i = *this;
            m_index = m_block->next_used_index_from(m_index + 1);
            m_ptr = m_block->cell_from_index(m_index);
            return i;
        }

        Cell *&operator*() {
            return m_ptr;
        }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_block == i2.m_block && i1.m_index == i2.m_index;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_block != i2.m_block || i1.m_index != i2.m_index;
        }

    private:
        // cannot be heap-allocated, because the GC is not aware of it.
        void *operator new(size_t size) = delete;

        HeapBlock *m_block;
        Cell *m_ptr { nullptr };
        size_t m_index { 0 };
    };

    iterator begin() {
        auto index = next_used_index_from(0);
        return iterator { this, index };
    }

    iterator end() {
        return iterator { this, m_total_count };
    }

    void unmark_all_cells() {
        for (auto cell : *this) {
            cell->unmark();
        }
    }

private:
    // returns -1 if it's a bad pointer or doesn't belong to this block
    ssize_t index_from_cell(const Cell *cell) const {
        auto diff = reinterpret_cast<const char *>(cell) - m_memory;
        if (diff < 0 || diff % m_cell_size != 0)
            return -1;
        auto index = diff / m_cell_size;
        if (index >= m_total_count)
            return -1;
        return index;
    }

    // returns m_total_count if no more cells remain
    size_t next_used_index_from(size_t index) {
        while (index < m_total_count) {
            if (m_used_map[index])
                break;
            ++index;
        }
        return index;
    }

    FreeCellNode *m_free_list { nullptr };
    size_t m_cell_size;
    size_t m_total_count { 0 };
    size_t m_free_count { 0 };
    bool m_used_map[HEAP_CELL_COUNT_MAX] {};
    alignas(Cell) char m_memory[];
};

}
