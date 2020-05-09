#pragma once

#include "natalie.h"

#define NAT_HEAP_BLOCK_CELL_COUNT 200
#define NAT_HEAP_MIN_AVAIL_RATIO 0.1
#define NAT_HEAP_MIN_AVAIL_AFTER_COLLECTION_RATIO 0.2

struct NatHeapBlock {
    NatHeapBlock *next;
    NatObject storage[NAT_HEAP_BLOCK_CELL_COUNT];
    NatObject *free_list;
};

void nat_gc_init(NatEnv *env, void *bottom_of_stack);
NatHeapBlock *nat_gc_alloc_heap_block(NatGlobalEnv *global_env);
NatObject *nat_gc_malloc(NatEnv *env);
bool nat_gc_is_heap_ptr(NatEnv *env, NatObject *ptr);
NatObject *nat_gc_gather_roots(NatEnv *env);
void nat_gc_unmark_all_objects(NatEnv *env);
NatObject *nat_gc_mark_live_objects(NatEnv *env);
void nat_gc_collect(NatEnv *env);
void nat_gc_collect_all(NatEnv *env);
double nat_gc_cells_available_ratio(NatEnv *env);
NatObject *nat_alloc(NatEnv *env, NatObject *klass, enum NatValueType type);
