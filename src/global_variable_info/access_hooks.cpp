#include "natalie/global_variable_info/access_hooks.hpp"
#include "natalie.hpp"

namespace Natalie {

namespace GlobalVariableAccessHooks::ReadHooks {

    Optional<Value> getpid(Env *env, GlobalVariableInfo &info) {
        auto pid = Value::integer(::getpid());
        info.set_object(env, pid);
        info.set_read_hook(nullptr);
        return pid;
    }

    Optional<Value> last_exception(Env *env, GlobalVariableInfo &) {
        return env->exception_object();
    }

    Optional<Value> last_exception_backtrace(Env *env, GlobalVariableInfo &) {
        if (!env->exception_object().is_exception())
            return {};
        return env->exception_object().as_exception()->backtrace(env);
    }

    Optional<Value> last_match(Env *env, GlobalVariableInfo &) {
        return env->last_match();
    }

    Optional<Value> last_match_pre_match(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match.is_nil())
            return {};
        return last_match.as_match_data()->pre_match(env);
    }

    Optional<Value> last_match_post_match(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match.is_nil())
            return {};
        return last_match.as_match_data()->post_match(env);
    }

    Optional<Value> last_match_last_group(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match.is_nil())
            return {};
        return last_match.as_match_data()->captures(env).as_array()->compact(env).as_array()->last();
    }

    Optional<Value> last_match_to_s(Env *env, GlobalVariableInfo &) {
        auto last_match = env->last_match();
        if (last_match.is_nil())
            return {};
        return last_match.as_match_data()->to_s(env);
    }

}

namespace GlobalVariableAccessHooks::WriteHooks {

    Value as_string_or_raise(Env *env, Value v, GlobalVariableInfo &info) {
        if (v.is_nil())
            return Value::nil();
        if (!v.is_string())
            env->raise("TypeError", "value of {} must be String", info.name());
        return v.as_string();
    }

    Value as_string_or_raise_and_deprecated(Env *env, Value v, GlobalVariableInfo &info) {
        v = as_string_or_raise(env, v, info);
        return deprecated(env, v, info);
    }

    Value deprecated(Env *env, Value v, GlobalVariableInfo &info) {
        if (!v.is_nil())
            env->deprecation_warn("'{}' is deprecated", info.name());
        return v;
    }

    Value to_int(Env *env, Value v, GlobalVariableInfo &) {
        return v.to_int(env);
    }

    Value last_match(Env *env, Value v, GlobalVariableInfo &) {
        if (v.is_nil())
            return Value::nil();
        if (!v.is_match_data())
            env->raise("TypeError", "wrong argument type {} (expected MatchData)", v.klass()->inspect_module());
        auto match = v.as_match_data();
        env->set_last_match(match);
        return match;
    }

    Value set_stdout(Env *env, Value v, GlobalVariableInfo &) {
        if (!v.respond_to(env, "write"_s))
            env->raise("TypeError", "$stdout must have write method, {} given", v.klass()->inspect_module());
        return v;
    }

    Value set_verbose(Env *env, Value v, GlobalVariableInfo &) {
        GlobalEnv::the()->set_verbose(v.is_truthy());
        if (v.is_nil())
            return Value::nil();
        return bool_object(v.is_truthy());
    }
}

}
