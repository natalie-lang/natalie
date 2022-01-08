#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class FalseObject : public Object {
public:
    static FalseObject *the() {
        if (s_instance) {
            assert(s_instance->flags() == 0);
            return s_instance;
        }
        s_instance = new FalseObject();
        return s_instance;
    }

    bool and_method(Env *, Value);
    bool or_method(Env *, Value);
    Value to_s(Env *);

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<FalseObject %p>", this);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static FalseObject *s_instance = nullptr;

    FalseObject()
        : Object { Object::Type::False, GlobalEnv::the()->Object()->const_fetch("FalseClass"_s)->as_class() } { }
};

}
