#pragma once

#include "natalie/block.hpp"
#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/gc.hpp"

namespace Natalie {

struct Method : public gc {
    Method(Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *))
        : fn { fn }
        , undefined { !fn } { }

    Method(Block *block)
        : env { *block->env() } {
        block->copy_fn_pointer_to_method(this);
        env.caller = nullptr;
    }

    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env {};
    bool undefined { false };
};

}
