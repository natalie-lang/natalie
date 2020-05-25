#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *FalseClass_to_s(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_FALSE);
    NAT_ASSERT_ARGC(0);
    return string(env, "false");
}

}
