#pragma once

#include <assert.h>

#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class VoidPObject : public Object {
public:
    using CleanupFnPtr = void (*)(VoidPObject *);

    VoidPObject(void *ptr, CleanupFnPtr cleanup_fn = nullptr)
        : Object { Object::Type::VoidP, GlobalEnv::the()->Object() }
        , m_void_ptr { ptr }
        , m_cleanup_fn { cleanup_fn } { }

    virtual ~VoidPObject() override {
        if (m_cleanup_fn)
            m_cleanup_fn(this);
    }

    void *void_ptr() { return m_void_ptr; }
    void set_void_ptr(void *ptr) { m_void_ptr = ptr; }

    CleanupFnPtr cleanup_fn() { return m_cleanup_fn; }

private:
    void *m_void_ptr { nullptr };
    CleanupFnPtr m_cleanup_fn { nullptr };
};

}
