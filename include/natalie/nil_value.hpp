#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class NilValue : public Value {
public:
    static NilValue *the() {
        if (s_instance) {
            assert(s_instance->flags() == 0);
            return s_instance;
        }
        s_instance = new NilValue();
        return s_instance;
    }

    ValuePtr eqtilde(Env *, ValuePtr);
    ValuePtr to_s(Env *);
    ValuePtr to_a(Env *);
    ValuePtr to_i(Env *);
    ValuePtr inspect(Env *);

    virtual void gc_inspect(char *buf, size_t len) override {
        snprintf(buf, len, "<NilValue %p>", this);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static NilValue *s_instance = nullptr;

    NilValue()
        : Value { Value::Type::Nil, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("NilClass"))->as_class() } { }
};

}
