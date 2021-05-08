#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <setjmp.h>
#include <vector>

/*
 * TODO:
 * - remove these stubs below
 * - add destructors on every object
 */

class gc { };
struct GC_stack_base {
    void *mem_base;
};
#define GC_MALLOC malloc
#define GC_REALLOC realloc
#define GC_FREE free
#define GC_STRDUP strdup
#define GC_set_stackbottom(...) (void)0
#define GC_get_my_stackbottom(...) (void)0

#include "natalie/gc/allocator.hpp"
#include "natalie/gc/cell.hpp"
#include "natalie/gc/heap.hpp"
#include "natalie/gc/heap_block.hpp"
#include "natalie/gc/marking_visitor.hpp"
