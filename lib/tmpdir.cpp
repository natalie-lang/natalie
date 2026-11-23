#include "natalie.hpp"

#include <stdlib.h>

using namespace Natalie;

Value Dir_mktmpdir(Env *env, Value self, Args &&args, Block *) {
    if (args.size() != 0)
        env->raise("ArgumentError", "TODO: Support arguments");
    String tmpdir = self->send(env, "tmpdir"_s).as_string()->string();
    tmpdir.append("/XXXXXX");
    if (!mkdtemp(&tmpdir[0]))
        env->raise_errno();
    auto result = StringObject::create(std::move(tmpdir));
    return result;
}

Value Dir_tmpdir(Env *env, Value self, Args &&args, Block *) {
    args.ensure_argc_is(env, 0);

    const auto tmpdir = getenv("TMPDIR");
    if (tmpdir)
        return StringObject::create(tmpdir, Encoding::ASCII_8BIT);
    return find_top_level_const(env, "Etc"_s).send(env, "systmpdir"_s, std::move(args));
}

Value init_tmpdir(Env *env, Value self) {
    return Value::nil();
}
