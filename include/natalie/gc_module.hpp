#pragma once

#include "natalie/forward.hpp"
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
};

}
