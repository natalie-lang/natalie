#include "natalie/finalizer.hpp"
#include "natalie/value.hpp"

namespace Natalie {

void finalizer::finalize(void *obj, void *) {
    static_cast<ValuePtr >(obj)->~Value();
}

}
