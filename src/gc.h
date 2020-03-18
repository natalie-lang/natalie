#pragma once

#include "natalie.h"

#define NAT_BLOCK_SIZE 16384 // 16 KiB

NatObject *nat_alloc(NatEnv *env);
void *nat_malloc(NatEnv *env, size_t size);
void *nat_calloc(NatEnv *env, size_t count, size_t size);
void *nat_realloc(NatEnv *env, void *ptr, size_t size);
