#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Proc_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    return proc_new(env, block);
}

Value *Proc_call(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Proc);
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, self->block, argc, args, block);
}

Value *Proc_lambda(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Proc);
    if (self->lambda) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
