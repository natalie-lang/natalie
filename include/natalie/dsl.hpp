#pragma once

#include "env.hpp"
#include "global_env.hpp"
#include "string.hpp"
#include "symbol_value.hpp"
#include "value_ptr.hpp"

namespace Natalie {

inline ValuePtr find_const(Env *env, SymbolValue *path) {
    return GlobalEnv::the()->Object()->const_find(env, path);
}

inline ValuePtr find_nested_const(Env *env, std::initializer_list<SymbolValue *> paths) {
    ValuePtr c;
    for (auto path : paths) {
        if (c == nullptr) {
            c = find_const(env, path);
            continue;
        }
        c = c->const_find(env, path);
    }
    return c;
}

}