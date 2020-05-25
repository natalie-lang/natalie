#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Encoding_list(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *ary = array_new(env);
    array_push(env, ary, const_get(env, self, "ASCII_8BIT", true));
    array_push(env, ary, const_get(env, self, "UTF_8", true));
    return ary;
}

Value *Encoding_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return sprintf(env, "#<Encoding:%S>", vector_get(&self->encoding_names->ary, 0));
}

Value *Encoding_name(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return static_cast<Value *>(vector_get(&self->encoding_names->ary, 0));
}

Value *Encoding_names(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_ENCODING);
    return self->encoding_names;
}

}
