#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/symbol_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class TrueValue : public Value {
public:
    static TrueValue *the() {
        if (s_instance) {
            assert(s_instance->flags() == 0);
            return s_instance;
        }
        s_instance = new TrueValue();
        return s_instance;
    }

    ValuePtr to_s(Env *);

    virtual void gc_inspect(char *buf, size_t len) override {
        snprintf(buf, len, "<TrueValue %p>", this);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static TrueValue *s_instance = nullptr;

    TrueValue()
        : Value { Value::Type::True, GlobalEnv::the()->Object()->const_fetch(SymbolValue::intern("TrueClass"))->as_class() } { }
};

}
