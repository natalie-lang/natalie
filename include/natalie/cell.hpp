#pragma once

#include "gc/gc_cpp.h"

namespace Natalie {

// The BDWGC gc_cleanup class implements this, but it does so using
// ordered finalizers and complains loudly when there are cycles among
// objects. Since none of our destructors should depend on linked objects,
// we can implement our own gc_cleanup (called Cell) that uses
// GC_register_finalizer_no_order to avoid any cycles during finalization.
class Cell : public gc {
public:
    virtual ~Cell() {
        void *base = GC_base(this);
        if (0 == base) return; // Non-heap object.
        GC_register_finalizer_no_order(base, 0, 0, 0, 0);
    }

    static void cleanup(void *obj, void *displ) {
        reinterpret_cast<class gc_cleanup *>(reinterpret_cast<char *>(obj) + reinterpret_cast<GC_PTRDIFF_T>(displ))->~gc_cleanup();
    }

    void gc_cleanup() {
        GC_finalization_proc oldProc = 0;
        void *oldData = 0;
        void *this_ptr = reinterpret_cast<void *>(this);
        void *base = GC_base(this_ptr);
        if (base != 0) {
            GC_register_finalizer_no_order(base,
                reinterpret_cast<GC_finalization_proc>(cleanup),
                reinterpret_cast<void *>(reinterpret_cast<char *>(this_ptr) - reinterpret_cast<char *>(base)),
                &oldProc, &oldData);
            if (oldProc != 0) {
                GC_register_finalizer_no_order(base, oldProc, oldData, 0, 0);
            }
        }
    }

    void *operator new(GC_SIZE_T size) {
        void *obj = GC_MALLOC(size);
        if (obj != 0)
            GC_REGISTER_FINALIZER_NO_ORDER(obj, cleanup, nullptr, 0, 0);
        GC_OP_NEW_OOM_CHECK(obj);
        return obj;
    }

    void *operator new(GC_SIZE_T size, void *obj) {
        if (obj != 0)
            GC_REGISTER_FINALIZER_NO_ORDER(obj, cleanup, nullptr, 0, 0);
        GC_OP_NEW_OOM_CHECK(obj);
        return obj;
    }

    void operator delete(void *obj) { GC_FREE(obj); }
};

}
