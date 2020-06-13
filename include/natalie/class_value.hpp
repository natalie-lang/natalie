#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/module_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ClassValue : ModuleValue {
    ClassValue(Env *env)
        : ClassValue { env, NAT_OBJECT->const_get(env, "Class", true)->as_class() } { }

    ClassValue *subclass(Env *, const char * = nullptr);

    static ClassValue *bootstrap_class_class(Env *);
    static ClassValue *bootstrap_basic_object(Env *, ClassValue *);

private:
    ClassValue(Env *env, ClassValue *klass)
        : ModuleValue { env, Value::Type::Class, klass } { }
};

}
