#include "natalie.hpp"
#include "natalie/fiber_value.hpp"

namespace Natalie {

FiberValue *GlobalEnv::main_fiber(Env *env) {
    if (m_main_fiber) return m_main_fiber;
    m_main_fiber = new FiberValue { env };
    return m_main_fiber;
}

}
