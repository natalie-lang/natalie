#include "natalie/gc_module.hpp"

namespace Natalie {

bool GCModule::disable() {
    Heap::the().gc_disable();
    return true;
}

}
