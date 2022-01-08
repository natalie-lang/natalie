#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class NilObject : public Object {
public:
    static NilObject *the() {
        if (s_instance) {
            assert(s_instance->flags() == 0);
            return s_instance;
        }
        s_instance = new NilObject();
        return s_instance;
    }

    bool and_method(Env *, Value);
    bool or_method(Env *, Value);
    Value eqtilde(Env *, Value);
    Value to_s(Env *);
    Value to_a(Env *);
    Value to_h(Env *);
    Value to_f(Env *);
    Value to_i(Env *);
    Value inspect(Env *);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<NilObject %p>", this);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static NilObject *s_instance = nullptr;

    NilObject()
        : Object { Object::Type::Nil, GlobalEnv::the()->Object()->const_fetch("NilClass"_s)->as_class() } { }
};

}
