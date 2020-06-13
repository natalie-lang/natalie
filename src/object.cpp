#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *Object_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj = new Value { self->as_class() };
    return obj->initialize(env, argc, args, block);
}

}
