#pragma once
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <assert.h>
#include <mutex>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "gc/gc.h"
#include "natalie/macros.hpp"
#include "tm/hashmap.hpp"
#include "tm/vector.hpp"

namespace Natalie {

using namespace TM;

static void ignore_warning(char *msg, GC_word arg) { }

class Heap {
public:
    NAT_MAKE_NONCOPYABLE(Heap);

    static Heap &the() {
        if (s_instance)
            return *s_instance;
        s_instance = new Heap();
        return *s_instance;
    }

    void collect() const { GC_gcollect(); }

    bool gc_enabled() const { return !GC_is_disabled(); }

    void gc_enable() const {
        auto warn_proc = GC_get_warn_proc();
        GC_set_warn_proc(ignore_warning);
        GC_enable();
        GC_set_warn_proc(warn_proc);
    }

    void gc_disable() const { GC_disable(); }

    bool collect_all_at_exit() const { return m_collect_all_at_exit; }
    void set_collect_all_at_exit(bool collect) { m_collect_all_at_exit = collect; }

private:
    inline static Heap *s_instance = nullptr;

    Heap() { }

    // FIXME
    bool m_collect_all_at_exit { false };
};

}
