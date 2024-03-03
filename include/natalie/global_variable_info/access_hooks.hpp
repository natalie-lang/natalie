#pragma once

#include "natalie/global_variable_info.hpp"

namespace Natalie {

namespace GlobalVariableAccessHooks::ReadHooks {
    Value getpid(Env *, GlobalVariableInfo &);
    Value last_match(Env *, GlobalVariableInfo &);
    Value last_match_pre_match(Env *, GlobalVariableInfo &);
    Value last_match_post_match(Env *, GlobalVariableInfo &);
    Value last_match_last_group(Env *, GlobalVariableInfo &);
    Value last_match_to_s(Env *, GlobalVariableInfo &);
}

namespace GlobalVariableAccessHooks::WriteHooks {
    Object *to_int(Env *, Value, GlobalVariableInfo &);
    Object *last_match(Env *, Value, GlobalVariableInfo &);
}

}
