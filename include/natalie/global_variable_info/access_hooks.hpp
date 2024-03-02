#pragma once

#include "natalie/global_variable_info.hpp"

namespace Natalie {

namespace GlobalVariableAccessHooks::ReadHooks {
    Value getpid(Env *, GlobalVariableInfo &);
}

namespace GlobalVariableAccessHooks::WriteHooks {
    Object *to_int(Env *, Value, GlobalVariableInfo &);
}

}
