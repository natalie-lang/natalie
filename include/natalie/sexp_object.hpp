#pragma once

#include "natalie.hpp"

namespace Natalie {

class SexpObject : public ArrayObject {
public:
    SexpObject(Env *, Node *, std::initializer_list<Value>);

    Value new_method(Env *env, size_t argc, Value *args) {
        auto sexp = new SexpObject {};
        sexp->ivar_set(env, SymbolObject::intern("@file"), ivar_get(env, SymbolObject::intern("@file")));
        sexp->ivar_set(env, SymbolObject::intern("@line"), ivar_get(env, SymbolObject::intern("@line")));
        for (size_t i = 0; i < argc; i++) {
            sexp->push(args[i]);
        }
        return sexp;
    }

    Value inspect(Env *env) {
        StringObject *out = new StringObject { "s(" };
        for (size_t i = 0; i < size(); i++) {
            Value obj = (*this)[i];
            StringObject *repr = obj.send(env, SymbolObject::intern("inspect"))->as_string();
            out->append(env, repr);
            if (i < size() - 1) {
                out->append(env, ", ");
            }
        }
        out->append_char(')');
        return out;
    }

    Value file(Env *env) { return ivar_get(env, SymbolObject::intern("@file")); }
    Value set_file(Env *env, Value file) { return ivar_set(env, SymbolObject::intern("@file"), file); }

    Value line(Env *env) { return ivar_get(env, SymbolObject::intern("@line")); }
    Value set_line(Env *env, Value line) { return ivar_set(env, SymbolObject::intern("@line"), line); }

private:
    SexpObject(std::initializer_list<Value> list)
        : ArrayObject { list } {
        m_klass = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Sexp"))->as_class();
    }
};

}
