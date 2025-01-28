#include "natalie.hpp"

#include <stdlib.h>

using namespace Natalie;

Value Dir_tmpdir(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    const auto tmpdir = getenv("TMPDIR");
    if (tmpdir)
        return new StringObject { tmpdir, Encoding::ASCII_8BIT };
    return Etc_systmpdir(env, self, std::move(args), nullptr);
}

Value init_tmpdir(Env *env, Value self) {
    return NilObject::the();
}
