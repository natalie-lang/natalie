#include "natalie.hpp"

namespace Natalie {

Value ComplexObject::inspect(Env *env) {
    return StringObject::format("({}+{}i)", m_real.inspect_str(env), m_imaginary.inspect_str(env));
}

}
