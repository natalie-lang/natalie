#pragma once

#include "natalie/env.hpp"
#include "natalie/forward.hpp"

namespace Natalie {

struct Block {

    Block(Env &env, Value *self, Value *(*fn)(Env *, Value *, ssize_t, Value **, Block *))
        : fn { fn }
        , env { env }
        , self { self } {
        this->env.caller = nullptr;
    }

    // NOTE: This should only be called from one of the RUN_BLOCK_* macros!
    Value *_run(Env *env, ssize_t argc, Value **args, Block *block) {
        Env e = Env::new_block_env(&this->env, env);
        return this->fn(&e, self, argc, args, block);
    }

    Value *(*fn)(Env *env, Value *self, ssize_t argc, Value **args, Block *block) { nullptr };
    Env env {};
    Value *self { nullptr };
};

}
