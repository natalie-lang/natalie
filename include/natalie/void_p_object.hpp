#pragma once

#include <assert.h>
#include <functional>

#include "natalie/class_object.hpp"
#include "natalie/encoding_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class VoidPObject : public Object {
public:
    using CleanupFnPtr = std::function<void(VoidPObject *)>;

    VoidPObject(void *ptr)
        : Object { Object::Type::VoidP, GlobalEnv::the()->Object() }
        , m_void_ptr { ptr } { }

    VoidPObject(void *ptr, CleanupFnPtr cleanup_fn)
        : Object { Object::Type::VoidP, GlobalEnv::the()->Object() }
        , m_void_ptr { ptr }
        , m_cleanup_fn { cleanup_fn } { }

    virtual ~VoidPObject() override {
        // FIXME: Allocating an object here can cause the GC to deadlock,
        // since an allocation can trigger a second nested GC, which cannot
        // work. We may need to disable the GC inside the destructor here,
        // or teach the GC to recognize and ignore a nested collection.
        if (m_cleanup_fn)
            m_cleanup_fn.value()(this);
    }

    void *void_ptr() { return m_void_ptr; }
    void set_void_ptr(void *ptr) { m_void_ptr = ptr; }

    Optional<CleanupFnPtr> cleanup_fn() { return m_cleanup_fn; }

private:
    void *m_void_ptr { nullptr };
    Optional<CleanupFnPtr> m_cleanup_fn;
};

}
