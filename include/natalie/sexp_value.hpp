#pragma once

#include "natalie.hpp"

namespace Natalie {

struct SexpValue : ArrayValue {
    SexpValue(Env *env, std::initializer_list<Value *> list)
        : ArrayValue { env, list } {
        m_klass = env->Object()->const_fetch("Parser")->const_fetch("Sexp")->as_class();
    }

    Value *inspect(Env *env) {
        StringValue *out = new StringValue { env, "s(" };
        for (size_t i = 0; i < size(); i++) {
            Value *obj = (*this)[i];
            StringValue *repr = obj->send(env, "inspect")->as_string();
            out->append_string(env, repr);
            if (i < size() - 1) {
                out->append(env, ", ");
            }
        }
        out->append_char(env, ')');
        return out;
    }
};

}
