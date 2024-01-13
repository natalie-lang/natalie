#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"

namespace Natalie {

class TrueObject : public Object {
public:
    static TrueObject *the() {
        if (s_instance) {
            // make sure we aren't accidentally changing flags
            assert(s_instance->flags() == Flag::Frozen);
            return s_instance;
        }
        auto mem = GC_MALLOC_UNCOLLECTABLE(sizeof(TrueObject));
        s_instance = new (mem) TrueObject();
        s_instance->freeze();
        return s_instance;
    }

    bool and_method(const Env *, Value) const;
    bool or_method(const Env *, Value) const;
    bool xor_method(const Env *, Value) const;
    Value to_s(const Env *) const;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<TrueObject %p>", this);
    }

private:
    inline static TrueObject *s_instance = nullptr;
    inline static StringObject *s_string = nullptr;

    TrueObject()
        : Object { Object::Type::True, GlobalEnv::the()->Object()->const_fetch("TrueClass"_s)->as_class() } { }
};

}
