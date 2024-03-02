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

}

namespace GlobalVariableAccessHooks::WriteHooks {

    Object *to_int(Env *env, Value v, GlobalVariableInfo &) {
        return v->to_int(env);
    }
}

}
