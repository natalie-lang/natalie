#pragma once

#include "env.hpp"
#include "global_env.hpp"
#include "string.hpp"
#include "symbol_object.hpp"
#include "value.hpp"

namespace Natalie {

inline Value find_const(Env *env, SymbolObject *path) {
    return GlobalEnv::the()->Object()->const_find(env, path);
}

inline Value find_nested_const(Env *env, std::initializer_list<SymbolObject *> paths) {
    Value c;
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
