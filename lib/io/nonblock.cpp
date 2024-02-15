#include "natalie.hpp"
#include <fcntl.h>

using namespace Natalie;

Value init_nonblock(Env *env, Value self) {
    return NilObject::the();
}

Value IO_is_nonblock(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 0);
    const auto io = self->as_io()->fileno();
    const auto flags = fcntl(io, F_GETFL);
    if (flags < 0)
        env->raise_errno();
    if (flags & O_NONBLOCK)
        return TrueObject::the();
    return FalseObject::the();
}

Value IO_set_nonblock(Env *env, Value self, Args args, Block *) {
    args.ensure_argc_is(env, 1);
    const auto io = self->as_io()->fileno();
    auto flags = fcntl(io, F_GETFL);
    if (flags < 0)
        env->raise_errno();
    if (args.at(0)->is_truthy()) {
        if ((flags & O_NONBLOCK) != O_NONBLOCK) {
            flags |= O_NONBLOCK;
            if (fcntl(io, F_SETFL, flags) < 0)
                env->raise_errno();
        }
    } else {
        if (flags & O_NONBLOCK) {
            flags &= ~O_NONBLOCK;
            if (fcntl(io, F_SETFL, flags) < 0)
                env->raise_errno();
        }
    }
    return args.at(0);
}
