#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class ParserObject : public Object {
public:
    ValuePtr parse(Env *, ValuePtr, ValuePtr = nullptr);
    ValuePtr tokens(Env *, ValuePtr, ValuePtr);
};

}
