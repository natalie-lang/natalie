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
            // make sure we aren't accidentally changing flags
            assert(s_instance->flags() == Flag::Frozen);
            return s_instance;
        }
        auto mem = GC_MALLOC_UNCOLLECTABLE(sizeof(FalseObject));
        s_instance = new (mem) FalseObject();
        s_instance->freeze();
        return s_instance;
    }

    bool and_method(const Env *, Value) const;
    bool or_method(const Env *, Value) const;
    Value to_s(const Env *) const;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<FalseObject %p>", this);
    }

private:
    inline static FalseObject *s_instance = nullptr;
    inline static StringObject *s_string = nullptr;

    FalseObject()
        : Object { Object::Type::False, GlobalEnv::the()->Object()->const_fetch("FalseClass"_s)->as_class() } { }
};

}
