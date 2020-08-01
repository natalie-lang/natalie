#include <stdarg.h>

#include "natalie.hpp"

namespace Natalie {

void Block::copy_fn_pointer_to_method(Method *method) {
    method->set_fn(m_fn);
}

}
