#include <stdexcept>

#include "natalie.hpp"

extern "C" void GC_disable() {
    Natalie::Heap::the().gc_disable();
}

namespace Natalie {

void *Cell::operator new(size_t size) {
    auto *cell = Heap::the().allocate(size);
    assert(cell);
    return cell;
}

void Cell::operator delete(void *ptr) {
    // We may need this in the future, but for now, let's just get a big
    // beautiful abort if anything is trying to delete a GC-allocated object.
    NAT_UNREACHABLE();
}

void MarkingVisitor::visit(Value val) {
    visit(val.object_or_null());
}

TM::Hashmap<Cell *> Heap::gather_conservative_roots() {
    void *dummy;
    void *end_of_stack = &dummy;

    // step over stack, saving potential pointers
    assert(m_start_of_stack);
    assert(end_of_stack);
    assert(m_start_of_stack > end_of_stack);

    Hashmap<Cell *> roots;

    for (char *ptr = reinterpret_cast<char *>(end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr); // NOLINT
        if (roots.get(potential_cell))
            continue;
        if (is_a_heap_cell_in_use(potential_cell)) {
            roots.set(potential_cell);
        }
    }

    // step over any registers, saving potential pointers
    jmp_buf jump_buf;
    setjmp(jump_buf);
    for (char *i = (char *)jump_buf; i < (char *)jump_buf + sizeof(jump_buf); ++i) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(i);
        if (roots.get(potential_cell))
            continue;
        if (is_a_heap_cell_in_use(potential_cell)) {
            roots.set(potential_cell);
        }
    }

    return roots;
}

void Heap::collect() {
    if (!m_gc_enabled) return;

    static auto is_profiled = NativeProfiler::the()->enabled();

    NativeProfilerEvent *collect_profiler_event;
    NativeProfilerEvent *mark_profiler_event;

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

    auto roots = gather_conservative_roots();

    for (auto pair : roots) {
        visitor.visit(pair.first);
    }

    visitor.visit(GlobalEnv::the());
    visitor.visit(NilObject::the());
    visitor.visit(TrueObject::the());
    visitor.visit(FalseObject::the());
    visitor.visit(FiberObject::main());
    visitor.visit(FiberObject::current());

    if (is_profiled)
        mark_profiler_event->end_now();

    sweep();
}

void Heap::sweep() {
    static auto is_profiled = NativeProfiler::the()->enabled();
    NativeProfilerEvent *profiler_event;
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
    static auto is_profiled = NativeProfiler::the()->enabled();
    NativeProfilerEvent *profiler_event;
    if (is_profiled)
        profiler_event = NativeProfilerEvent::named(NativeProfilerEvent::Type::ALLOCATE, "Allocate")->start_now();
    Defer log_event([&]() {
        if (is_profiled)
            NativeProfiler::the()->push(profiler_event->end_now());
    });
    auto &allocator = find_allocator_of_size(size);

    if (m_gc_enabled) {
#ifdef NAT_GC_DEBUG_ALWAYS_COLLECT
        collect();
#else

        if (allocator.total_cells() == 0) {
            allocator.add_multiple_blocks(initial_blocks_per_allocator);
        } else if (allocator.free_cells_percentage() < min_percent_free_triggers_collection) {
            collect();
            allocator.add_blocks_until_percent_free_reached(min_percent_free_after_collection);
        }
#endif
    }

    return allocator.allocate();
}

void Heap::return_cell_to_free_list(Cell *cell) {
    auto *block = HeapBlock::from_cell(cell);
    assert(is_a_heap_block(block));
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

Cell *HeapBlock::find_next_free_cell() {
    assert(has_free());
    --m_free_count;
    auto node = m_free_list;
    assert(node);
    m_free_list = node->next;
    assert(m_free_list || m_free_count == 0);
    auto cell = reinterpret_cast<Cell *>(node);
    m_used_map[node->index] = true;
    // printf("Cell %p allocated from block %p (size %zu) at index %zu\n", cell, this, m_cell_size, i);
    new (cell) Cell();
    return cell;
}

void HeapBlock::return_cell_to_free_list(const Cell *cell) {
    auto index = index_from_cell(cell);
    assert(index > -1);
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
