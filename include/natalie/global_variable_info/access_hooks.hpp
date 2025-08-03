#pragma once

#include "natalie/global_variable_info.hpp"

namespace Natalie {

namespace GlobalVariableAccessHooks::ReadHooks {
    Optional<Value> getpid(Env *, GlobalVariableInfo &);
    Optional<Value> last_exception(Env *, GlobalVariableInfo &);
    Optional<Value> last_exception_backtrace(Env *, GlobalVariableInfo &);
    Optional<Value> last_match(Env *, GlobalVariableInfo &);
    Optional<Value> last_match_pre_match(Env *, GlobalVariableInfo &);
    Optional<Value> last_match_post_match(Env *, GlobalVariableInfo &);
    Optional<Value> last_match_last_group(Env *, GlobalVariableInfo &);
    Optional<Value> last_match_to_s(Env *, GlobalVariableInfo &);
}

namespace GlobalVariableAccessHooks::WriteHooks {
    Value as_string_or_raise(Env *, Value, GlobalVariableInfo &);
    Value as_string_or_raise_and_deprecated(Env *, Value, GlobalVariableInfo &);
    Value deprecated(Env *, Value, GlobalVariableInfo &);
    Value to_int(Env *, Value, GlobalVariableInfo &);
    Value last_match(Env *, Value, GlobalVariableInfo &);
    Value set_stdout(Env *, Value, GlobalVariableInfo &);
    Value set_verbose(Env *, Value, GlobalVariableInfo &);
}

}
