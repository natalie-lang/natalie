#include <signal.h>
#include <stdexcept>

#include "natalie.hpp"

extern "C" void GC_disable() {
    Natalie::Heap::the().gc_disable();
}

namespace Natalie {

std::recursive_mutex g_gc_recursive_mutex;

void *Cell::operator new(size_t size) {
    return Heap::the().allocate(size);
}

void Cell::operator delete(void *) {
    // sweep() deletes cells, and this delete will almost never be called by generated code.
    // However, it does get called in one place that I know of... when creating a
    // RegexpObject, if the syntax is incorrect, an error is raised. Throwing an
    // exception causes C++ to automatically call delete on the in-progress
    // object creation. We can just ignore that and let sweep() clean up the cell later.
}

void MarkingVisitor::visit(const Value val) {
    if (val.is_pointer())
        visit(val.pointer());
}

#ifdef __SANITIZE_ADDRESS__
NO_SANITIZE_ADDRESS void Heap::visit_roots_from_asan_fake_stack(Cell::Visitor &visitor, Cell *potential_cell) {
    void *begin_fake_frame = nullptr;
    void *end_fake_frame = nullptr;
    auto fake_stack = __asan_get_current_fake_stack();
    void *real_stack = __asan_addr_is_in_fake_stack(fake_stack, potential_cell, &begin_fake_frame, &end_fake_frame);

    if (!real_stack) return;

    scan_memory(visitor, begin_fake_frame, end_fake_frame);
}
#else
void Heap::visit_roots_from_asan_fake_stack(Cell::Visitor &visitor, Cell *potential_cell) { }
#endif

NO_SANITIZE_ADDRESS void Heap::visit_roots(Cell::Visitor &visitor) {
    void *dummy;
    void *end_of_stack = &dummy;

    // step over stack, saving potential pointers
    auto start_of_stack = ThreadObject::current()->start_of_stack();
    assert(start_of_stack > end_of_stack);
#ifdef __SANITIZE_ADDRESS__
    scan_memory(visitor, end_of_stack, start_of_stack, [&](Cell *potential_cell) {
        visit_roots_from_asan_fake_stack(visitor, potential_cell);
    });
#else
    scan_memory(visitor, end_of_stack, start_of_stack);
#endif

    // step over any registers, saving potential pointers
    jmp_buf jump_buf;
    setjmp(jump_buf);
    auto start = reinterpret_cast<std::byte *>(jump_buf);
    scan_memory(visitor, start, start + sizeof(jump_buf));
}

void Heap::collect() {
    // Only collect on the main thread for now.
    if (!ThreadObject::current()->is_main()) return;

    std::lock_guard<std::recursive_mutex> gc_lock(g_gc_recursive_mutex);

    ThreadObject::stop_the_world_and_save_context();

    static auto is_profiled = NativeProfiler::the()->enabled();

    NativeProfilerEvent *collect_profiler_event = nullptr;
    NativeProfilerEvent *mark_profiler_event = nullptr;

    if (is_profiled) {
        collect_profiler_event = NativeProfilerEvent::named(NativeProfilerEvent::Type::GC, "GC_Collect")->start_now();
        mark_profiler_event = NativeProfilerEvent::named(NativeProfilerEvent::Type::GC, "GC_Mark");
    }

    Defer log_event([&]() {
        if (!is_profiled)
            return;
        NativeProfiler::the()->push(collect_profiler_event->end_now());
        NativeProfiler::the()->push(mark_profiler_event);
    });

    if (is_profiled) {
        mark_profiler_event->start_now();
    }

    MarkingVisitor visitor;

    visit_roots(visitor);

    visitor.visit(GlobalEnv::the());
    visitor.visit(Value::nil());
    visitor.visit(Value::True());
    visitor.visit(Value::False());
    visitor.visit(tl_current_exception);
    for (auto thread : ThreadObject::list())
        visitor.visit(thread);

    // We don't collect symbols, but they each can have associated objects we do collect.
    SymbolObject::visit_all_symbols(visitor);
    NilMethods::visit_string(visitor);
    TrueMethods::visit_string(visitor);
    FalseMethods::visit_string(visitor);

    if (is_profiled)
        mark_profiler_event->end_now();

    visitor.visit_all();

    ThreadObject::wake_up_the_world();

    sweep();
}

void Heap::sweep() {
    static auto is_profiled = NativeProfiler::the()->enabled();
    NativeProfilerEvent *profiler_event = nullptr;
    if (is_profiled)
        profiler_event = NativeProfilerEvent::named(NativeProfilerEvent::Type::GC, "GC_Sweep")->start_now();
    Defer log_event([&]() {
        if (is_profiled)
            NativeProfiler::the()->push(profiler_event->end_now());
    });
    for (auto allocator : m_allocators) {
        for (auto block_pair : *allocator) {
            auto *block = block_pair.first;
            for (auto cell : *block) {
                if (!cell->is_marked() && cell->is_collectible()) {
                    bool had_free = block->has_free();
                    block->return_cell_to_free_list(cell);
                    if (!had_free) {
                        allocator->add_free_block(block);
                    }
                } else {
                    cell->unmark();
                }
            }
        }
    }
}

void *Heap::allocate(size_t size) {
    std::lock_guard<std::recursive_mutex> gc_lock(g_gc_recursive_mutex);

    static auto is_profiled = NativeProfiler::the()->enabled();
    NativeProfilerEvent *profiler_event = nullptr;
    if (is_profiled)
        profiler_event = NativeProfilerEvent::named(NativeProfilerEvent::Type::ALLOCATE, "Allocate")->start_now();
    Defer log_event([&]() {
        if (is_profiled)
            NativeProfiler::the()->push(profiler_event->end_now());
    });

    auto &allocator = find_allocator_of_size(size);
    if (allocator.total_cells() == 0)
        allocator.add_multiple_blocks(initial_blocks_per_allocator);

    if (m_gc_enabled) {
#ifdef NAT_GC_DEBUG_ALWAYS_COLLECT
        collect();
#else

        if (m_allocations_without_collection_count++ >= check_free_percentage_every) {
            if (m_free_cells * 100 / m_total_cells < min_percent_free_triggers_collection)
                collect();
            m_allocations_without_collection_count = 0;
        }
#endif
    }

    return allocator.allocate();
}

void *Allocator::allocate() {
    Cell *cell = nullptr;
    if (m_free_blocks.empty()) {
        auto *block = add_heap_block();
        cell = block->find_next_free_cell();
    } else {
        auto *block = m_free_blocks.back();
        cell = block->find_next_free_cell();
        if (!block->has_free())
            m_free_blocks.pop_back();
    }
    --m_free_cells;
    --Heap::the().m_free_cells;
    return cell;
}

HeapBlock *Allocator::add_heap_block() {
    auto *block = reinterpret_cast<HeapBlock *>(aligned_alloc(HEAP_BLOCK_SIZE, HEAP_BLOCK_SIZE));
    new (block) HeapBlock(m_cell_size);
    m_blocks.set(block);
    add_free_block(block);
    m_free_cells += cell_count_per_block();
    Heap::the().m_free_cells += cell_count_per_block();
    Heap::the().m_total_cells += cell_count_per_block();
    return block;
}

void Heap::return_cell_to_free_list(Cell *cell) {
    auto *block = HeapBlock::from_cell(cell);
    block->return_cell_to_free_list(cell);
}

size_t Heap::total_allocations() const {
    size_t count = 0;
    for (auto allocator : m_allocators) {
        for (auto block_pair : *allocator) {
            auto *block = block_pair.first;
            count += block->used_count();
        }
    }
    return count;
}

void Heap::dump() const {
    nat_int_t allocation_count = 0;
    for (auto allocator : m_allocators) {
        for (auto block_pair : *allocator) {
            auto *block = block_pair.first;
            for (auto cell : *block) {
                cell->gc_print();
                allocation_count++;
            }
        }
    }
    printf("Total allocations: %lld\n", allocation_count);
}

NO_SANITIZE_ADDRESS void Heap::scan_memory(Cell::Visitor &visitor, void *start, void *end) {
    for (auto *ptr = reinterpret_cast<std::byte *>(start); ptr < end; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr); // NOLINT
        if (!potential_cell)
            continue;
        if (is_a_heap_cell_in_use(potential_cell))
            visitor.visit(potential_cell);
    }
}

NO_SANITIZE_ADDRESS void Heap::scan_memory(Cell::Visitor &visitor, void *start, void *end, std::function<void(Cell *)> fn) {
    for (auto *ptr = reinterpret_cast<std::byte *>(start); ptr < end; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr); // NOLINT
        if (!potential_cell)
            continue;
        if (is_a_heap_cell_in_use(potential_cell))
            visitor.visit(potential_cell);
        fn(potential_cell);
    }
}

Cell *HeapBlock::find_next_free_cell() {
    assert(has_free());
    --m_free_count;
    auto node = m_free_list;
    m_free_list = node->next;
    auto cell = reinterpret_cast<Cell *>(node);
    m_used_map[node->index] = true;
    // printf("Cell %p allocated from block %p (size %zu) at index %zu\n", cell, this, m_cell_size, node->index);
    new (cell) Cell();
    return cell;
}

void HeapBlock::return_cell_to_free_list(const Cell *cell) {
    // printf("returning %p to free list\n", cell);
    auto index = index_from_cell(cell);
    m_used_map[index] = false;
    cell->~Cell();
    memset(&m_memory[index * m_cell_size], 0, m_cell_size);
    auto node = reinterpret_cast<FreeCellNode *>(cell_from_index(index));
    node->next = m_free_list;
    node->index = index;
    m_free_list = node;
    ++m_free_count;
}
}
