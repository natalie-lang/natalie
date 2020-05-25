#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Proc_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    return proc_new(env, block);
}

NatObject *Proc_call(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_PROC);
    return NAT_RUN_BLOCK_WITHOUT_BREAK(env, self->block, argc, args, block);
}

NatObject *Proc_lambda(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_PROC);
    if (self->lambda) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

}
