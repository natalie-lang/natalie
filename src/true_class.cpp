#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *TrueClass_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    assert(self_value->is_true());
    NAT_ASSERT_ARGC(0);
    return string(env, "true");
}

}
