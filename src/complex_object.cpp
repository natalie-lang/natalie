#include "natalie.hpp"

namespace Natalie {

Value ComplexObject::inspect(Env *env) {
    return StringObject::format("({}+{}i)", m_real.inspected(env), m_imaginary.inspected(env));
}

}
