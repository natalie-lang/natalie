#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/io_object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/string_object.hpp"

namespace Natalie {

class EnvObject : public Object {
public:
    ValuePtr inspect(Env *);
    ValuePtr ref(Env *, ValuePtr name);
    ValuePtr refeq(Env *, ValuePtr name, ValuePtr value);
};

}
