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

class EnvValue : public Value {
public:
    ValuePtr inspect(Env *);
    ValuePtr ref(Env *, ValuePtr name);
    ValuePtr refeq(Env *, ValuePtr name, ValuePtr value);
};

}
