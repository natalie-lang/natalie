#include "natalie.hpp"
#include <fcntl.h>

using namespace Natalie;

Value init_nonblock(Env *env, Value self) {
    return NilObject::the();
}

Value IO_is_nonblock(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    if (self->as_io()->is_nonblock(env))
        return TrueObject::the();
    return FalseObject::the();
}

Value IO_set_nonblock(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    self->as_io()->set_nonblock(env, args.at(0)->is_truthy());
    return args.at(0);
}
