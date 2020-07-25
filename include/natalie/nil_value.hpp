#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct NilValue : Value {
    static NilValue *instance(Env *env) {
        if (env->nil_obj()) return env->nil_obj();
        return new NilValue { env };
    }

    Value *to_s(Env *);
    Value *to_a(Env *);
    Value *to_i(Env *);
    Value *inspect(Env *);

private:
    NilValue(Env *env)
        : Value { Value::Type::Nil, env->Object()->const_get(env, "NilClass", true)->as_class() } { }
};

}
