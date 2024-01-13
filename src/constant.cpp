#include "natalie.hpp"

namespace Natalie {

void Constant::autoload(Env *env, Value self) {
    assert(m_autoload_fn);
    auto fn = m_autoload_fn;
    m_autoload_fn = nullptr; // remove autoload fn so we don't have a possible loop
    fn(env, self, {}, nullptr);
}

}
