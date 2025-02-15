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
            assert(s_instance->is_frozen());
            return s_instance;
        }
        s_instance = new NilObject();
        s_instance->freeze();
        return s_instance;
    }

    static bool and_method(Value, Value);
    static bool or_method(Value, Value);
    static Value eqtilde(Value, Value);
    static Value rationalize(Value, Value);
    static Value to_s(Value);
    static Value to_a(Value);
    static Value to_c(Value);
    static Value to_h(Value);
    static Value to_f(Value);
    static Value to_i(Value);
    static Value to_r(Value);
    static Value inspect(Value);

    virtual void visit_children(Visitor &visitor) const override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<NilObject %p>", this);
    }

    virtual bool is_collectible() override {
        return false;
    }

private:
    inline static NilObject *s_instance = nullptr;
    inline static StringObject *s_string = nullptr;

    NilObject()
        : Object { Object::Type::Nil, GlobalEnv::the()->Object()->const_fetch("NilClass"_s).as_class() } { }
};

}
