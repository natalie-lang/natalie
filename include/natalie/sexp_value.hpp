#pragma once

#include "natalie.hpp"

namespace Natalie {

class SexpValue : public ArrayValue {
public:
    SexpValue(Env *, Node *, std::initializer_list<ValuePtr>);

    ValuePtr new_method(Env *env, size_t argc, ValuePtr *args) {
        auto sexp = new SexpValue {};
        sexp->ivar_set(env, SymbolValue::intern("@file"), ivar_get(env, SymbolValue::intern("@file")));
        sexp->ivar_set(env, SymbolValue::intern("@line"), ivar_get(env, SymbolValue::intern("@line")));
        for (size_t i = 0; i < argc; i++) {
            sexp->push(args[i]);
        }
        return sexp;
    }

    ValuePtr inspect(Env *env) {
        StringValue *out = new StringValue { "s(" };
        for (size_t i = 0; i < size(); i++) {
            ValuePtr obj = (*this)[i];
            StringValue *repr = obj.send(env, SymbolValue::intern("inspect"))->as_string();
            out->append(env, repr);
            if (i < size() - 1) {
                out->append(env, ", ");
            }
        }
        out->append_char(env, ')');
        return out;
    }

    ValuePtr file(Env *env) { return ivar_get(env, SymbolValue::intern("@file")); }
    ValuePtr set_file(Env *env, ValuePtr file) { return ivar_set(env, SymbolValue::intern("@file"), file); }

    ValuePtr line(Env *env) { return ivar_get(env, SymbolValue::intern("@line")); }
    ValuePtr set_line(Env *env, ValuePtr line) { return ivar_set(env, SymbolValue::intern("@line"), line); }

private:
    SexpValue(std::initializer_list<ValuePtr> list)
        : ArrayValue { list } {
        m_klass = GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("Sexp"))->as_class();
    }
};

}
