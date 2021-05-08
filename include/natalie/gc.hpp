#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <setjmp.h>
#include <vector>

#include <natalie/macros.hpp>

/*
 * TODO:
 * - remove these stubs below
 * - add destructors on every object
 */

class gc { };
struct GC_stack_base {
    void *mem_base;
};
#define GC_MALLOC malloc
#define GC_REALLOC realloc
#define GC_FREE free
#define GC_STRDUP strdup
#define GC_set_stackbottom(...) (void)0
#define GC_get_my_stackbottom(...) (void)0

namespace Natalie {

struct ValuePtr;

constexpr const size_t HEAP_BLOCK_SIZE = 32 * 1024;
constexpr const size_t HEAP_BLOCK_MASK = ~(HEAP_BLOCK_SIZE - 1);
constexpr const size_t HEAP_CELL_COUNT_MAX = HEAP_BLOCK_SIZE / 16; // 16 bytes is the smallest cell we will allocate

class Cell {
public:
    Cell() { }
    virtual ~Cell() { }

    void *operator new(size_t size);

    class Visitor {
    public:
        virtual void visit(Cell *) = 0;
        virtual void visit(ValuePtr) = 0;
    };

    void virtual visit_children(Visitor &) = 0;

    bool marked() {
        return m_marked;
    }

    void mark() {
        m_marked = true;
    }

    void unmark() {
        m_marked = false;
    }

private:
    bool m_marked { false };
};

class MarkingVisitor : public Cell::Visitor {
public:
    virtual void visit(Cell *cell) override final {
        if (!cell || cell->marked()) return;
        cell->mark();
        cell->visit_children(*this);
    }

    virtual void visit(ValuePtr val) override final;
};

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
        // FIXME: could use pointer arithmetic instead of iterating
        for (size_t i = 0; i < m_total_count; ++i) {
            const void *comparison_cell = &m_memory[i * m_cell_size];
            if (cell == comparison_cell)
                return m_used_map[i];
        }
        return false;
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
    size_t m_cell_size;
    size_t m_total_count { 0 };
    size_t m_free_count { 0 };
    bool m_used_map[HEAP_CELL_COUNT_MAX] {};
    alignas(Cell) char m_memory[];
};

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
        if (m_blocks.empty())
            return 0;
        return free_cells() * 100 / total_cells();
    }

    void *allocate() {
        for (auto block : m_blocks) {
            if (block->has_free()) {
                --m_free_cells;
                return block->next_free();
            }
        }
        auto *block = add_heap_block();
        return block->next_free();
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

private:
    HeapBlock *add_heap_block() {
        auto *block = reinterpret_cast<HeapBlock *>(aligned_alloc(HEAP_BLOCK_SIZE, HEAP_BLOCK_SIZE));
        new (block) HeapBlock(m_cell_size);
        m_blocks.push_back(block);
        m_free_cells += cell_count_per_block();
        return block;
    }

    size_t m_cell_size;
    size_t m_free_cells { 0 };
    std::vector<HeapBlock *> m_blocks;
};

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
        MarkingVisitor visitor;
        auto roots = gather_conservative_roots();
        for (auto cell : roots) {
            cell->visit_children(visitor);
        }
        sweep();
    }

    void init(void *bottom_of_stack) {
        m_bottom_of_stack = bottom_of_stack;
    }

private:
    static Heap *s_instance;

    Heap() {
        m_allocators.push_back(new Allocator(16));
        m_allocators.push_back(new Allocator(32));
        m_allocators.push_back(new Allocator(64));
        m_allocators.push_back(new Allocator(128));
        m_allocators.push_back(new Allocator(256));
        m_allocators.push_back(new Allocator(512));
        m_allocators.push_back(new Allocator(1024));
    }

    ~Heap() {
        for (auto *allocator : m_allocators) {
            delete allocator;
        }
    }

    std::vector<Cell *> gather_conservative_roots() {
        void *dummy;
        void *top_of_stack = &dummy;
        std::vector<Cell *> possible_cells;
        std::vector<Cell *> roots;

        jmp_buf jump_buf;
        setjmp(jump_buf);
        void *raw_jump_buf = reinterpret_cast<void *>(jump_buf);

        for (size_t i = 0; i < ((size_t)sizeof(jump_buf)) / sizeof(intptr_t); i += sizeof(intptr_t)) {
            void *offset = (reinterpret_cast<void **>(raw_jump_buf))[i];
            if (offset == nullptr)
                continue;
            possible_cells.push_back(reinterpret_cast<Cell *>(offset));
        }

        for (char *stack_address = reinterpret_cast<char *>(top_of_stack); stack_address < m_bottom_of_stack; stack_address += sizeof(intptr_t)) {
            possible_cells.push_back(reinterpret_cast<Cell *>(stack_address));
        }

        for (auto *candidate_cell : possible_cells) {
            auto *candidate_block = HeapBlock::from_cell(candidate_cell);
            if (is_a_heap_block(candidate_block) && candidate_block->cell_in_use(candidate_cell))
                roots.push_back(candidate_cell);
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

    std::vector<Allocator *> m_allocators;

    void *m_bottom_of_stack { nullptr };
};

}
