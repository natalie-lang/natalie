#pragma once

#include "natalie.h"

#define NAT_BLOCK_SIZE 16384 // 16 KiB

NatObject *nat_alloc(NatEnv *env);
