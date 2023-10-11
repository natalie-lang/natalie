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
            assert(s_instance->flags() == Flag::Frozen);
            return s_instance;
        }
        s_instance = new NilObject();
        s_instance->freeze();
        return s_instance;
    }

    bool and_method(const Env *, Value) const;
    bool or_method(const Env *, Value) const;
    Value eqtilde(const Env *, Value) const;
    Value rationalize(const Env *, Value) const;
    Value to_s(const Env *) const;
    Value to_a(const Env *) const;
    Value to_c(const Env *) const;
    Value to_h(const Env *) const;
    Value to_f(const Env *) const;
    Value to_i(const Env *) const;
    Value to_r(const Env *) const;
    Value inspect(const Env *) const;

    virtual void visit_children(Visitor &visitor) override final;

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
        : Object { Object::Type::Nil, GlobalEnv::the()->Object()->const_fetch("NilClass"_s)->as_class() } { }
};

}
