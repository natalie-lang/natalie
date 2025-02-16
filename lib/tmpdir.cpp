#include "natalie.hpp"

#include <stdio.h>
#include <stdlib.h>

using namespace Natalie;

Value Dir_mktmpdir(Env *env, Value self, Args &&args, Block *) {
    if (args.size() != 0)
        env->raise("ArgumentError", "TODO: Support arguments");
    String tmpdir = self->send(env, "tmpdir"_s).as_string()->string();
    tmpdir.append("/XXXXXX");
    auto fh = mkstemp(&tmpdir[0]);
    if (fh < 0)
        env->raise_errno();
    close(fh);
    unlink(tmpdir.c_str());
    auto result = new StringObject { std::move(tmpdir) };
    self->send(env, "mkdir"_s, { result });
    return result;
}

Value Dir_tmpdir(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    const auto tmpdir = getenv("TMPDIR");
    if (tmpdir)
        return new StringObject { tmpdir, Encoding::ASCII_8BIT };
    return Etc_systmpdir(env, self, std::move(args), nullptr);
}

Value init_tmpdir(Env *env, Value self) {
    return Value::nil();
}
