#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include <natalie/macros.hpp>

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

const int HEAP_BLOCK_CELL_COUNT = 1024;

class Cell;

class HeapBlock {
public:
    HeapBlock(size_t size)
        : m_cell_size { size } {
        m_memory = new char[size * HEAP_BLOCK_CELL_COUNT];
    }

    ~HeapBlock() {
        // TODO: assert that all the objects are deleted already?
        delete[] m_memory;
    }

    NAT_MAKE_NONCOPYABLE(HeapBlock);

    bool has_free() {
        return m_free_count > 0;
    }

    void *next_free() {
        assert(has_free());
        for (size_t i = 0; i < HEAP_BLOCK_CELL_COUNT; i++) {
            if (!m_used_map[i]) {
                m_used_map[i] = true;
                --m_free_count;
                void *offset = &m_memory[i * m_cell_size];
                return reinterpret_cast<void *>(offset);
            }
        }
        NAT_UNREACHABLE();
    }

private:
    size_t m_cell_size;
    size_t m_free_count { HEAP_BLOCK_CELL_COUNT };
    bool m_used_map[HEAP_BLOCK_CELL_COUNT] {};
    char *m_memory { nullptr };
};

class Allocator {
public:
    NAT_MAKE_NONCOPYABLE(Allocator);

    Allocator(size_t size)
        : m_size { size } {
    }

    ~Allocator() {
        for (auto &block : m_blocks) {
            delete block;
        }
    }

    size_t size() {
        return m_size;
    }

    void *allocate() {
        for (auto block : m_blocks) {
            if (block->has_free()) {
                return block->next_free();
            }
        }
        auto *block = add_heap_block();
        return block->next_free();
    }

private:
    HeapBlock *add_heap_block() {
        auto *block = new HeapBlock(m_size);
        m_blocks.push_back(block);
        return block;
    }

    size_t m_size;
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
        return allocator.allocate();
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

    Allocator &find_allocator_of_size(size_t size) {
        for (auto *allocator : m_allocators) {
            if (allocator->size() >= size) {
                return *allocator;
            }
        }
        abort();
    }

    std::vector<Allocator *> m_allocators;
};

class Cell {
public:
    Cell() { }
    virtual ~Cell() { }

    void *operator new(size_t size) {
        auto *cell = Heap::the().allocate(size);
        assert(cell);
        return cell;
    }
};

}
