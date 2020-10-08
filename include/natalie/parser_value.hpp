#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct ParserValue : Value {
    Value *parse(Env *, Value *);
};

}
