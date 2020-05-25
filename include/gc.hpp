#pragma once

#include "natalie.hpp"

namespace Natalie {

#define NAT_HEAP_BLOCK_SIZE 262144 // 2 ^ 18
#define NAT_HEAP_MIN_AVAIL_RATIO 0.1
#define NAT_HEAP_MIN_AVAIL_AFTER_COLLECTION_RATIO 0.2
#define NAT_HEAP_MIN_CELL_SIZE 161

#define NAT_HEAP_BLOCK_HEADER_SIZE sizeof(NatHeapBlock)
#define NAT_HEAP_CELL_HEADER_SIZE sizeof(NatHeapCell)

#define NAT_HEAP_BLOCK_FIRST_CELL(block) (NatHeapCell *)((char *)block + NAT_HEAP_BLOCK_HEADER_SIZE)
#define NAT_HEAP_CELL_START_USABLE(cell) (void *)((char *)cell + NAT_HEAP_CELL_HEADER_SIZE)

#define NAT_HEAP_CELL_FROM_OBJ(obj) (NatHeapCell *)((char *)obj - NAT_HEAP_CELL_HEADER_SIZE)
#define NAT_HEAP_OBJ_FROM_CELL(cell) (void *)((char *)cell + NAT_HEAP_CELL_HEADER_SIZE)

struct NatHeapBlock {
    NatHeapBlock *next;
    NatHeapCell *free_list;
    NatHeapCell *used_list;
};

struct NatHeapCell {
    ssize_t size;
    bool mark;
    NatHeapCell *next;
};

void nat_gc_init(NatEnv *env, void *bottom_of_stack);
NatHeapBlock *nat_gc_alloc_heap_block(NatGlobalEnv *global_env);
NatObject *nat_gc_malloc(NatEnv *env);
void nat_gc_collect(NatEnv *env);
void nat_gc_collect_all(NatEnv *env);
double nat_gc_bytes_available_ratio(NatEnv *env);
NatObject *nat_alloc(NatEnv *env, NatObject *klass, enum NatValueType type);

extern void *nat_gc_abort_if_collected;

}
