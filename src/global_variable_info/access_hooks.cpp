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

    Value last_exception(Env *env, GlobalVariableInfo &) {
        return env->exception_object();
    }

    Value last_exception_backtrace(Env *env, GlobalVariableInfo &) {
        if (!env->exception_object()->is_exception())
            return NilObject::the();
        return env->exception_object()->as_exception()->backtrace(env);
    }

    Value last_match(Env *env, GlobalVariableInfo &) {
        return env->last_match();
    }

    Value last_match_pre_match(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match->is_nil())
            return NilObject::the();
        return last_match->as_match_data()->pre_match(env);
    }

    Value last_match_post_match(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match->is_nil())
            return NilObject::the();
        return last_match->as_match_data()->post_match(env);
    }

    Value last_match_last_group(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match->is_nil())
            return NilObject::the();
        return last_match->as_match_data()->captures(env)->as_array()->compact(env)->as_array()->last();
    }

    Value last_match_to_s(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match->is_nil())
            return NilObject::the();
        return last_match->as_match_data()->to_s(env);
    }

}

namespace GlobalVariableAccessHooks::WriteHooks {

    Object *as_string_or_raise(Env *env, Value v, GlobalVariableInfo &) {
        if (v->is_nil())
            return NilObject::the();
        return v->as_string_or_raise(env);
    }

    Object *to_int(Env *env, Value v, GlobalVariableInfo &) {
        return v->to_int(env);
    }

    Object *last_match(Env *env, Value v, GlobalVariableInfo &) {
        if (!v || v->is_nil())
            return NilObject::the();
        auto match = v->as_match_data_or_raise(env);
        env->set_last_match(match);
        return match;
    }

    Object *set_stdout(Env *env, Value v, GlobalVariableInfo &) {
        if (!v->respond_to(env, "write"_s))
            env->raise("TypeError", "$stdout must have write method, {} given", v->klass()->inspect_str());
        return v.object();
    }

    Object *set_verbose(Env *env, Value v, GlobalVariableInfo &) {
        GlobalEnv::the()->set_verbose(v->is_truthy());
        if (v->is_nil())
            return NilObject::the();
        return bool_object(v->is_truthy()).object();
    }
}

}
