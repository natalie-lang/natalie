#pragma once

#include "natalie/forward.hpp"
#include "natalie/hash_object.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class GCModule : public Object {
public:
    static bool disable();
    static bool enable();
    static Value dump();

    static Value start(Env *env) {
#ifndef NAT_GC_DISABLE
        Heap::the().collect();
#endif
        return Value::nil();
    }

    // prints stats from the GC
    // just the allocation count, for now,
    // but we can print more stuff later...
    static bool print_stats(Env *env) {
        auto count = Heap::the().total_allocations();
        printf("allocations: %zu\n", count);
        return true; // return bool so we don't allocate anything new
    }
};

}
