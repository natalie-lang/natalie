#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Encoding_list(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *ary = array_new(env);
    array_push(env, ary, const_get(env, self, "ASCII_8BIT", true));
    array_push(env, ary, const_get(env, self, "UTF_8", true));
    return ary;
}

NatObject *Encoding_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return sprintf(env, "#<Encoding:%S>", vector_get(&self->encoding_names->ary, 0));
}

NatObject *Encoding_name(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return static_cast<NatObject *>(vector_get(&self->encoding_names->ary, 0));
}

NatObject *Encoding_names(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return self->encoding_names;
}

}
