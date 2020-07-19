#include <stdarg.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

void Block::copy_fn_pointer_to_method(Method *method) {
    method->fn = m_fn;
}

}
