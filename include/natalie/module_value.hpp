#pragma once

#include <assert.h>

#include "natalie/env.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ModuleValue : Value {
    ModuleValue(Env *);
    ModuleValue(Env *, const char *);
    ModuleValue(Env *, Type, ClassValue *);

    char *class_name { nullptr };
    ClassValue *superclass { nullptr };
    struct hashmap methods EMPTY_HASHMAP;
    struct hashmap cvars EMPTY_HASHMAP;
    ssize_t included_modules_count { 0 };
    ModuleValue **included_modules { nullptr };
};

}
