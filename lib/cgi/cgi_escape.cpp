#include "natalie.hpp"

using namespace Natalie;

Value init_cgi_escape(Env *env, Value self) { return Value::nil(); }

Value CGI_to_str(Env *env, Value string) { return string.to_str2(env); }
