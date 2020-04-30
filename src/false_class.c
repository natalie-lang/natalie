#include "builtin.h"
#include "natalie.h"

NatObject *FalseClass_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_FALSE);
    NAT_ASSERT_ARGC(0);
    return nat_string(env, "false");
}
