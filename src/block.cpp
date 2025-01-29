#include <stdarg.h>

#include "natalie.hpp"

namespace Natalie {

Value Block::run(Env *env, Args &&args, Block *block) {
    Env e { m_env };
    e.set_caller(env);
    e.set_this_block(this);
    args.pop_empty_keyword_hash();
    auto fiber = Natalie::FiberObject::current();
    Value result;
    do {
        result = m_fn(&e, m_self, std::move(args), block);
    } while (fiber->check_redo_block_and_clear());
    return result;
}

void Block::copy_fn_pointer_to_method(Method *method) {
    method->set_fn(m_fn);
}

}
