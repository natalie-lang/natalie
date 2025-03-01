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
            assert(s_instance->is_frozen());
            return s_instance;
        }
        s_instance = new TrueObject();
        s_instance->freeze();
        return s_instance;
    }

    static bool and_method(const Env *, Value, Value);
    static bool or_method(const Env *, Value, Value);
    static bool xor_method(const Env *, Value, Value);
    static Value to_s(const Env *, Value);

    virtual void visit_children(Visitor &visitor) const override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<TrueObject %p>", this);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static TrueObject *s_instance = nullptr;
    inline static StringObject *s_string = nullptr;

    TrueObject()
        : Object { Object::Type::True, GlobalEnv::the()->Object()->const_fetch("TrueClass"_s).as_class() } { }
};

}
