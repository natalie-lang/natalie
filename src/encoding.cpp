#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Encoding_list(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *ary = new ArrayValue { env };
    ClassValue *self = self_value->as_class();
    ary->push(self->const_get(env, "ASCII_8BIT", true));
    ary->push(self->const_get(env, "UTF_8", true));
    return ary;
}

Value *Encoding_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    EncodingValue *self = self_value->as_encoding();
    return StringValue::sprintf(env, "#<Encoding:%S>", self->name());
}

Value *Encoding_name(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    EncodingValue *self = self_value->as_encoding();
    return const_cast<StringValue *>(self->name());
}

Value *Encoding_names(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    EncodingValue *self = self_value->as_encoding();
    ArrayValue *names = new ArrayValue { env };
    for (Value *name : *self->names(env)) {
        names->push(name);
    }
    return names;
}

}
