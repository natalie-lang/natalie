#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class SignalModule : public Object {
public:
    static Value list(Env *);
    static Value signame(Env *, Value);
};

}
