#include "natalie.hpp"

namespace Natalie {

Value ComplexObject::imaginary(Env *env) {
    return m_imaginary;
}

Value ComplexObject::inspect(Env *env) {
    return StringObject::format("({}+{}i)", m_real->inspect_str(env), m_imaginary->inspect_str(env));
}

Value ComplexObject::real(Env *env) {
    return m_real;
}
}
