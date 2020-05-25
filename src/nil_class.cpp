#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *NilClass_to_s(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_NIL);
    NAT_ASSERT_ARGC(0);
    NatObject *out = string(env, "");
    return out;
}

NatObject *NilClass_to_a(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_NIL);
    NAT_ASSERT_ARGC(0);
    return array_new(env);
}

NatObject *NilClass_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_NIL);
    NAT_ASSERT_ARGC(0);
    NatObject *out = string(env, "nil");
    return out;
}

NatObject *NilClass_is_nil(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_NIL);
    NAT_ASSERT_ARGC(0);
    return NAT_TRUE;
}

}
