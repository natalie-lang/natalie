#pragma once

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"

namespace Natalie {

struct Method {
    Method(Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *))
        : fn { fn }
        , undefined { !fn } { }

    Method(Block *block)
        : fn { block->fn }
        , env { block->env }
        , undefined { false } {
        env.caller = nullptr;
    }

    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env {};
    bool undefined { false };
};

}
