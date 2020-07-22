#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *ProcValue::initialize(Env *env, Block *block) {
    m_block = block;
    return this;
}

Value *ProcValue::call(Env *env, ssize_t argc, Value **args, Block *block) {
    assert(m_block);
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_block, argc, args, block);
}

}
