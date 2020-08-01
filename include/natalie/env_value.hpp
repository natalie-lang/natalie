#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/io_value.hpp"
#include "natalie/regexp_value.hpp"
#include "natalie/string_value.hpp"

namespace Natalie {

struct EnvValue : Value {
    Value *inspect(Env *);
    Value *ref(Env *, Value *name);
    Value *refeq(Env *, Value *name, Value *value);
};

}
