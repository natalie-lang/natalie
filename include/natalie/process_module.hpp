#pragma once

#include <sys/types.h>
#include <unistd.h>

#include "natalie/forward.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class ProcessModule : public Value {
public:
    static ValuePtr pid(Env *env) {
        pid_t pid = getpid();
        return ValuePtr::integer(pid);
    }
};

}
