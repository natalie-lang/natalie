#include "natalie/gc_module.hpp"

namespace Natalie {

bool GCModule::disable() {
    if (!Heap::the().gc_enabled())
        return true;
    Heap::the().gc_disable();
    return false;
}

bool GCModule::enable() {
    if (Heap::the().gc_enabled())
        return false;
    Heap::the().gc_enable();
    return true;
}

Value GCModule::dump() {
    Heap::the().dump();
    return Value::nil();
}

}
