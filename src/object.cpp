#include "natalie/builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Object_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj = new Value { env, self->as_class() };
    return initialize(env, obj, argc, args, block);
}

}
