#pragma once

#include <string.h>

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

namespace Natalie {

class Heap {
public:
    Heap() { }
    ~Heap();
};

class HeapBlock {
};

class Cell {
public:
    Cell() { }
    virtual ~Cell() { }

    void *operator new(size_t size) {
        return ::operator new(size);
    }
};

}
