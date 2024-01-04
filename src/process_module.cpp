#include "natalie.hpp"

#include <signal.h>
#include <time.h>

namespace Natalie {

Value ProcessModule::groups(Env *env) {
    auto size = getgroups(0, nullptr);
    if (size < 0)
        env->raise_errno();
    gid_t list[size];
    size = getgroups(size, list);
    if (size < 0)
        env->raise_errno();
    auto result = new ArrayObject { static_cast<size_t>(size) };
    for (size_t i = 0; i < static_cast<size_t>(size); i++) {
        result->push(IntegerObject::create(list[i]));
    }
    return result;
}

Value ProcessModule::clock_gettime(Env *env, Value clock_id) {
    timespec tp;
    const auto clk_id = static_cast<clockid_t>(IntegerObject::convert_to_nat_int_t(env, clock_id));
    if (::clock_gettime(clk_id, &tp) < 0)
        env->raise_errno();

    double result = static_cast<double>(tp.tv_sec) + tp.tv_nsec / static_cast<double>(1000000000);
    return new FloatObject { result };
}

Value ProcessModule::kill(Env *env, Args args) {
    env->ensure_no_extra_keywords(args.pop_keyword_hash());
    args.ensure_argc_at_least(env, 2);
    auto signal = args.shift();
    auto pids = args.to_array();
    nat_int_t signo;
    bool pid_contains_self = false;

    if (signal->is_symbol())
        signal = signal->to_s(env);
    if (signal->is_integer()) {
        signo = IntegerObject::convert_to_nat_int_t(env, signal);
    } else if (signal->is_string() || signal->respond_to(env, "to_str"_s)) {
        auto signame = signal->to_str(env)->delete_prefix(env, new StringObject { "SIG" });
        auto signo_val = SignalModule::list(env)->as_hash()->ref(env, signame);
        if (signo_val->is_nil())
            env->raise("ArgumentError", "unsupported signal `SIG{}'", signame->to_s(env)->string());
        signo = IntegerObject::convert_to_nat_int_t(env, signo_val);
    } else {
        env->raise("ArgumentError", "bad signal type {}", signal->klass()->inspect_str());
    }
    const auto own_pid = getpid();
    for (Value pid : *pids) {
        const auto int_pid = IntegerObject::convert_to_nat_int_t(env, pid);
        if (own_pid == int_pid && signo != 0) {
            pid_contains_self = true;
        } else {
            if (::kill(int_pid, signo) < 0)
                env->raise_errno();
        }
    }
    if (pid_contains_self) {
        auto signal_exception = GlobalEnv::the()->Object()->const_get("SignalException"_s)->as_class();
        auto exception = _new(env, signal_exception, { signal }, nullptr)->as_exception();
        env->raise_exception(exception);
    }
    return Value::integer(pids->size());
}

}
