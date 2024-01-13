#pragma once

#include "gc/gc.h"

#ifndef TM_CALLOC
#define TM_CALLOC(count, size) GC_malloc(count *size)
#define TM_REALLOC GC_realloc
#define TM_FREE (void)sizeof
#endif
