#include "natalie.hpp"

namespace Natalie {

Value ProcObject::initialize(Env *env, Block *block) {
    m_block = block;
    return this;
}

Value ProcObject::call(Env *env, size_t argc, Value *args, Block *block) {
    assert(m_block);
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_block, argc, args, block);
}

}
