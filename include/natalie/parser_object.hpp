#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class ParserObject : public Object {
public:
    Value parse(Env *, Value, Value = nullptr);
    Value tokens(Env *, Value, Value);
};

}
