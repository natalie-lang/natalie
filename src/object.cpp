#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"

namespace Natalie {

NatObject *Object_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *obj = alloc(env, self, NAT_VALUE_OTHER);
    return initialize(env, obj, argc, args, block);
}

}
