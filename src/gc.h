#pragma once

#include "natalie.h"

#define NAT_HEAP_OBJECT_COUNT 26

#define NAT_LOCK_ALLOC(env) { \
    int lock_err = pthread_mutex_lock(&env->global_env->alloc_mutex); \
    if (lock_err) { \
        fprintf(stderr, "Could not lock allocation mutex: %d\n", lock_err); \
        abort(); \
    } \
}

#define NAT_UNLOCK_ALLOC(env) { \
    int unlock_err = pthread_mutex_unlock(&env->global_env->alloc_mutex); \
    if (unlock_err) { \
        fprintf(stderr, "Could not unlock allocation mutex: %d\n", unlock_err); \
        abort(); \
    } \
}

struct NatHeapBlock {
    NatHeapBlock *next;
    NatObject storage[NAT_HEAP_OBJECT_COUNT];
    NatObject *free_list;
};

void nat_gc_init(NatEnv *env, void *bottom_of_stack);
NatHeapBlock *nat_gc_alloc_heap_block(NatGlobalEnv *global_env);
NatObject *nat_gc_malloc(NatEnv *env);
NatObject *nat_gc_gather_roots(NatEnv *env);
void nat_gc_unmark_all_objects(NatEnv *env);
NatObject *nat_gc_mark_live_objects(NatEnv *env);
void nat_gc_collect(NatEnv *env);
void nat_gc_collect_all(NatEnv *env);
NatObject *nat_alloc(NatEnv *env, NatObject *klass, enum NatValueType type);
