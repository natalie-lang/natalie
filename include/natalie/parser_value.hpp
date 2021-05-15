#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/value.hpp"

namespace Natalie {

class ParserValue : public Value {
public:
    ValuePtr parse(Env *, ValuePtr, ValuePtr = nullptr);
    ValuePtr tokens(Env *, ValuePtr, ValuePtr);
};

}
