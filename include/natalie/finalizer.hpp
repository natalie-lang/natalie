#pragma once

#include "natalie/gc.hpp"

namespace Natalie {

struct finalizer {
    finalizer() {
#ifndef NAT_GC_DISABLE
        GC_finalization_proc oldProc;
        void *oldData;
        void *this_ptr = (void *)this;
        void *base = GC_base(this_ptr);
        if (base != 0) {
            // Don't call the debug version, since this is a real base address.
            GC_register_finalizer_ignore_self(base, (GC_finalization_proc)finalize,
                (void *)((char *)this_ptr - (char *)base),
                &oldProc, &oldData);
            if (oldProc != 0) {
                GC_register_finalizer_ignore_self(base, oldProc, oldData, 0, 0);
            }
        }
#endif
    }

    static void finalize(void *obj, void *);
};

}
