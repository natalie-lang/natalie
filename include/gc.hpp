#pragma once

#include "natalie.hpp"

namespace Natalie {

#define NAT_HEAP_BLOCK_SIZE 262144 // 2 ^ 18
#define NAT_HEAP_MIN_AVAIL_RATIO 0.1
#define NAT_HEAP_MIN_AVAIL_AFTER_COLLECTION_RATIO 0.2
#define NAT_HEAP_MIN_CELL_SIZE 161

#define NAT_HEAP_BLOCK_HEADER_SIZE sizeof(HeapBlock)
#define NAT_HEAP_CELL_HEADER_SIZE sizeof(HeapCell)

#define NAT_HEAP_BLOCK_FIRST_CELL(block) (HeapCell *)((char *)block + NAT_HEAP_BLOCK_HEADER_SIZE)
#define NAT_HEAP_CELL_START_USABLE(cell) (void *)((char *)cell + NAT_HEAP_CELL_HEADER_SIZE)

#define NAT_HEAP_CELL_FROM_OBJ(obj) (HeapCell *)((char *)obj - NAT_HEAP_CELL_HEADER_SIZE)
#define NAT_HEAP_OBJ_FROM_CELL(cell) (void *)((char *)cell + NAT_HEAP_CELL_HEADER_SIZE)

struct HeapBlock {
    HeapBlock *next;
    HeapCell *free_list;
    HeapCell *used_list;
};

struct HeapCell {
    ssize_t size;
    bool mark;
    HeapCell *next;
};

void gc_init(Env *env, void *bottom_of_stack);
HeapBlock *gc_alloc_heap_block(GlobalEnv *global_env);
Value *gc_malloc(Env *env);
void gc_collect(Env *env);
void gc_collect_all(Env *env);
double gc_bytes_available_ratio(Env *env);
Value *alloc(Env *env, Value *klass, enum NatValueType type);

extern void *gc_abort_if_collected;

}
