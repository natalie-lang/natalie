#pragma once

#include "symbol_value.hpp"
#include "string.hpp"
#include "value_ptr.hpp"
#include "global_env.hpp"
#include "env.hpp"

namespace Natalie {

ValuePtr find_constant(Env *env, std::initializer_list<SymbolValue *> paths) 
{
    ValuePtr c;
    for (auto path : paths) {
        if (c == nullptr) {
            c = GlobalEnv::the()->Object()->const_find(env, path);
            continue;
        }
        c = c->const_find(env, path);
    }
    return c;
}

}