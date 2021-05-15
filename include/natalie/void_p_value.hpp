#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/encoding_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class VoidPValue : public Value {
public:
    VoidPValue(Env *env)
        : VoidPValue { env, nullptr } { }

    VoidPValue(Env *env, void *ptr)
        : Value { Value::Type::VoidP, env->Object() }
        , m_void_ptr { ptr } { }

    void *void_ptr() { return m_void_ptr; }
    void set_void_ptr(void *ptr) { m_void_ptr = ptr; }

private:
    void *m_void_ptr { nullptr };
};

}
