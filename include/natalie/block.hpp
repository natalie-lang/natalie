#pragma once

#include "natalie/env.hpp"
#include "natalie/forward.hpp"

namespace Natalie {

struct Block {
    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env;
    Value *self { nullptr };
};

}
