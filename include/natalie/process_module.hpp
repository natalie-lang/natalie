#pragma once

#include <sys/types.h>
#include <unistd.h>

#include "natalie/forward.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class ProcessModule : public Object {
public:
    static Value pid(Env *env) {
        pid_t pid = getpid();
        return Value::integer(pid);
    }
};

}
