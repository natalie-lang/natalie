#include "natalie.hpp"

namespace Natalie {

ValuePtr ProcValue::initialize(Env *env, Block *block) {
    m_block = block;
    return this;
}

ValuePtr ProcValue::call(Env *env, size_t argc, ValuePtr *args, Block *block) {
    assert(m_block);
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_block, argc, args, block);
}

}
