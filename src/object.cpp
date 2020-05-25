#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Object_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj = alloc(env, self, ValueType::Other);
    return initialize(env, obj, argc, args, block);
}

}
