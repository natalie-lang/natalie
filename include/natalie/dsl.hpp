#pragma once

#include "symbol_value.hpp"
#include "string.hpp"
#include "value_ptr.hpp"
#include "global_env.hpp"
#include "env.hpp"

namespace Natalie {

ValuePtr find_constant(Env *env, Vector<String> paths) 
{
    ValuePtr c;
    for (auto path : paths) {
        if (c == nullptr) {
            c = GlobalEnv::the()->Object()->const_find(env, SymbolValue::intern(&path));
            continue;
        }
        c = c->const_find(env, SymbolValue::intern(&path));
    }
    return c;
}
}

#define NAT_CONST(...) Natalie::find_constant(env, TM::Vector<Natalie::String> { __VA_ARGS__ })