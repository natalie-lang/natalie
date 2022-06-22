#include "natalie.hpp"

namespace Natalie {

Value ProcObject::initialize(Env *env, Block *block) {
    m_block = block;
    return this;
}

Value ProcObject::call(Env *env, Args args, Block *block) {
    assert(m_block);
    if (is_lambda() && m_break_point != 0) {
        try {
            return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_block, args, block);
        } catch (ExceptionObject *exception) {
            if (exception->is_local_jump_error_with_break_point(env, m_break_point))
                return exception->send(env, "exit_value"_s);
            throw exception;
        }
    }
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, m_block, args, block);
}

}
