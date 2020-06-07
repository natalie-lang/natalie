#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *NilClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    Value *out = string(env, "");
    return out;
}

Value *NilClass_to_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return array_new(env);
}

Value *NilClass_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    Value *out = string(env, "nil");
    return out;
}

Value *NilClass_is_nil(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return NAT_TRUE;
}

}
