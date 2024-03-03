#include "natalie/global_variable_info/access_hooks.hpp"
#include "natalie.hpp"

namespace Natalie {

namespace GlobalVariableAccessHooks::ReadHooks {

    Value getpid(Env *env, GlobalVariableInfo &info) {
        Object *pid = new IntegerObject { ::getpid() };
        info.set_object(env, pid);
        info.set_read_hook(nullptr);
        return pid;
    }

    Value last_match(Env *env, GlobalVariableInfo &) {
        return env->last_match();
    }

}

namespace GlobalVariableAccessHooks::WriteHooks {

    Object *to_int(Env *env, Value v, GlobalVariableInfo &) {
        return v->to_int(env);
    }

    Object *last_match(Env *env, Value v, GlobalVariableInfo &) {
        if (!v || v->is_nil())
            return NilObject::the();
        return v->as_match_data_or_raise(env);
    }
}

}
