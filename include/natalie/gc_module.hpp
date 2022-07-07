#pragma once

#include "natalie/forward.hpp"
#include "natalie/hash_object.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class GCModule : public Object {
public:
    static bool enable() {
        Heap::the().gc_enable();
        return true;
    }

    static bool disable() {
        Heap::the().gc_disable();
        return true;
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
