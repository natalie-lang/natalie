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

    static Value start(Env *env) {
        Heap::the().collect();
        return NilObject::the();
    }

    static bool print_stats(Env *env) {
        struct GC_prof_stats_s stats;
        GC_get_prof_stats(&stats, sizeof(stats));
        printf("heapsize_full = %zu\n", stats.heapsize_full);
        printf("free_bytes_full = %zu\n", stats.free_bytes_full);
        printf("unmapped_bytes = %zu\n", stats.unmapped_bytes);
        printf("bytes_allocd_since_gc = %zu\n", stats.bytes_allocd_since_gc);
        printf("allocd_bytes_before_gc = %zu\n", stats.allocd_bytes_before_gc);
        printf("non_gc_bytes = %zu\n", stats.non_gc_bytes);
        printf("gc_no = %zu\n", stats.gc_no);
        printf("markers_m1 = %zu\n", stats.markers_m1);
        printf("bytes_reclaimed_since_gc = %zu\n", stats.bytes_reclaimed_since_gc);
        printf("reclaimed_bytes_before_gc = %zu\n", stats.reclaimed_bytes_before_gc);
        printf("expl_freed_bytes_since_gc = %zu\n", stats.expl_freed_bytes_since_gc);
        printf("obtained_from_os_bytes = %zu\n", stats.obtained_from_os_bytes);
        return true;
    }
};

}
