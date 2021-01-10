#pragma once

#include "natalie.hpp"

namespace Natalie {

struct SexpValue : ArrayValue {
    SexpValue(Env *, Node *, std::initializer_list<ValuePtr >);

    ValuePtr new_method(Env *env, size_t argc, ValuePtr *args) {
        auto sexp = new SexpValue { env, {} };
        sexp->ivar_set(env, "@file", ivar_get(env, "@file"));
        sexp->ivar_set(env, "@line", ivar_get(env, "@line"));
        for (size_t i = 0; i < argc; i++) {
            sexp->push(args[i]);
        }
        return sexp;
    }

    ValuePtr inspect(Env *env) {
        StringValue *out = new StringValue { env, "s(" };
        for (size_t i = 0; i < size(); i++) {
            ValuePtr obj = (*this)[i];
            StringValue *repr = obj->send(env, "inspect")->as_string();
            out->append_string(env, repr);
            if (i < size() - 1) {
                out->append(env, ", ");
            }
        }
        out->append_char(env, ')');
        return out;
    }

    ValuePtr file(Env *env) { return ivar_get(env, "@file"); }
    ValuePtr set_file(Env *env, ValuePtr file) { return ivar_set(env, "@file", file); }

    ValuePtr line(Env *env) { return ivar_get(env, "@line"); }
    ValuePtr set_line(Env *env, ValuePtr line) { return ivar_set(env, "@line", line); }

private:
    SexpValue(Env *env, std::initializer_list<ValuePtr > list)
        : ArrayValue { env, list } {
        m_klass = env->Object()->const_fetch("Parser")->const_fetch("Sexp")->as_class();
    }
};

}
