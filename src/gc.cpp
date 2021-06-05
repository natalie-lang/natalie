#include "natalie.hpp"

#ifdef NAT_GC_FIND_BUGS_WRITE_BACKTRACE_FILES
#include <execinfo.h>
#include <fcntl.h>
#endif

extern "C" void GC_disable() {
    Natalie::Heap::the().gc_disable();
}

namespace Natalie {

Heap *Heap::s_instance = nullptr;

#ifdef NAT_GC_FIND_BUGS_WRITE_BACKTRACE_FILES
void write_backtrace_to_file(const char *path) {
    void *array[1000];
    size_t size = backtrace(array, 1000);
    auto fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("error opening %s\n", path);
    } else {
        backtrace_symbols_fd(array, size, fd);
        close(fd);
    }
}
#endif

void *Cell::operator new(size_t size, AllocationStrategy allocation_strategy) {
    auto *cell = Heap::the().allocate(size, allocation_strategy);
    assert(cell);
#ifdef NAT_GC_FIND_BUGS_WRITE_BACKTRACE_FILES
    char path[100];
    snprintf(path, 100, "/tmp/create_%p.txt", cell);
    write_backtrace_to_file(path);
#endif
    return cell;
}

void Cell::operator delete(void *ptr) {
    // We may need this in the future, but for now, let's just get a big
    // beautiful abort if anything is trying to delete a GC-allocated object.
    NAT_UNREACHABLE();
}

void MarkingVisitor::visit(ValuePtr val) {
    visit(val.value_or_null());
}

Hashmap<Cell *> Heap::gather_conservative_roots() {
    Hashmap<Cell *> roots;

    void *dummy;
    void *end_of_stack = &dummy;

    // step over stack, saving potential pointers
    assert(m_start_of_stack);
    assert(end_of_stack);
    assert(m_start_of_stack > end_of_stack);
    for (char *ptr = reinterpret_cast<char *>(end_of_stack); ptr < m_start_of_stack; ptr += sizeof(intptr_t)) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(ptr);
        if (is_a_heap_cell_in_use(potential_cell)) {
#ifdef NAT_GC_FIND_BUGS
            if (potential_cell->m_collected) continue;
#endif
            roots.set(potential_cell);
        }
    }

    // step over any registers, saving potential pointers
    jmp_buf jump_buf;
    setjmp(jump_buf);
    for (char *i = (char *)jump_buf; i < (char *)jump_buf + sizeof(jump_buf); ++i) {
        Cell *potential_cell = *reinterpret_cast<Cell **>(i);
        if (is_a_heap_cell_in_use(potential_cell)) {
#ifdef NAT_GC_FIND_BUGS
            if (potential_cell->m_collected) continue;
#endif
            roots.set(potential_cell);
        }
    }

    return roots;
}

void Heap::collect() {
    if (!m_gc_enabled) return;

    for (auto allocator : m_allocators) {
        allocator->unmark_all_cells_in_all_blocks();
    }

    MarkingVisitor visitor;

    auto roots = gather_conservative_roots();
    for (auto pair : roots) {
        visitor.visit(pair.first);
    }

    sweep();
}

void Heap::sweep() {
    for (auto allocator : m_allocators) {
        for (auto block : *allocator) {
            for (auto cell : *block) {
                if (!cell->is_marked() && cell->is_collectible()) {
#ifdef NAT_GC_FIND_BUGS_PRINT_COLLECTED_CELLS
                    cell->gc_print();
                    fprintf(stderr, "\n");
#endif
#ifdef NAT_GC_FIND_BUGS_WRITE_BACKTRACE_FILES
                    char path[100];
                    snprintf(path, 100, "/tmp/collect_%p.txt", cell);
                    write_backtrace_to_file(path);
#endif
                    block->return_cell_to_free_list(cell);
                }
            }
        }
    }
}

void *Heap::allocate(size_t size, AllocationStrategy allocation_strategy) {
    auto &allocator = find_allocator_of_size(size);

    if (m_gc_enabled && allocation_strategy == AllocationStrategy::Automatic) {
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
#ifdef NAT_GC_FIND_BUGS_WRITE_BACKTRACE_FILES
    char path[100];
    snprintf(path, 100, "/tmp/collect_%p.txt", cell);
    write_backtrace_to_file(path);
#endif
}

}
