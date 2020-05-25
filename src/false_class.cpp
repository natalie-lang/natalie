#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *FalseClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::False);
    NAT_ASSERT_ARGC(0);
    return string(env, "false");
}

}
