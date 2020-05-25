#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *TrueClass_to_s(NatEnv *env, NatObject *self, ssize_t argc, NatObject **args, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_TRUE);
    NAT_ASSERT_ARGC(0);
    return nat_string(env, "true");
}

}
