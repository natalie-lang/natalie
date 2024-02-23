#pragma once

#include "natalie/global_variable_info.hpp"

namespace Natalie {

namespace GlobalVariableAccessHooks::ReadHooks {
    Value getpid(Env *, GlobalVariableInfo &);
}

}
