#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <setjmp.h>

#include "natalie/gc/cell.hpp"
#include "natalie/macros.hpp"

namespace Natalie {

constexpr const size_t HEAP_BLOCK_SIZE = 32 * 1024;
constexpr const size_t HEAP_BLOCK_MASK = ~(HEAP_BLOCK_SIZE - 1);
constexpr const size_t HEAP_CELL_COUNT_MAX = HEAP_BLOCK_SIZE / 16; // 16 bytes is the smallest cell we will allocate

class HeapBlock {
public:
    NAT_MAKE_NONCOPYABLE(HeapBlock);

    HeapBlock(size_t size)
        : m_cell_size { size }
        , m_total_count { (HEAP_BLOCK_SIZE - sizeof(HeapBlock)) / m_cell_size }
        , m_free_count { m_total_count } {
        memset(m_memory, 0, HEAP_BLOCK_SIZE - sizeof(HeapBlock));
    }

    ~HeapBlock() { }

    static HeapBlock *from_cell(const Cell *cell) {
        return reinterpret_cast<HeapBlock *>((intptr_t)cell & HEAP_BLOCK_MASK);
    }

    bool has_free() {
        return m_free_count > 0;
    }

    void *next_free() {
        assert(has_free());
        for (size_t i = 0; i < m_total_count; ++i) {
            if (!m_used_map[i]) {
                m_used_map[i] = true;
                --m_free_count;
                void *cell = &m_memory[i * m_cell_size];
                return cell;
            }
        }
        NAT_UNREACHABLE();
    }

    bool cell_in_use(const Cell *cell) const {
        auto index = cell_index(cell);
        return m_used_map[index];
    }

    void free_cell(const Cell *cell) {
        auto index = cell_index(cell);
        m_used_map[index] = false;
    }

    class iterator {
    public:
        iterator(HeapBlock *block, size_t index)
            : m_block { block }
            , m_index { index } { }

        iterator operator++() {
            ++m_index;
            return *this;
        }

        friend bool operator==(const iterator &i1, const iterator &i2) {
            return i1.m_block == i2.m_block && i1.m_index == i2.m_index;
        }

        friend bool operator!=(const iterator &i1, const iterator &i2) {
            return i1.m_block != i2.m_block || i1.m_index != i2.m_index;
        }

    private:
        HeapBlock *m_block;
        size_t m_index { 0 };
    };

    iterator begin() {
        return iterator { this, 0 };
    }

    iterator end() {
        return iterator { this, HEAP_CELL_COUNT_MAX + 1 };
    }

private:
    size_t cell_index(const Cell *cell) const {
        return (reinterpret_cast<const char *>(cell) - m_memory) / m_cell_size;
    }

    size_t m_cell_size;
    size_t m_total_count { 0 };
    size_t m_free_count { 0 };
    bool m_used_map[HEAP_CELL_COUNT_MAX] {};
    alignas(Cell) char m_memory[];
};

}
