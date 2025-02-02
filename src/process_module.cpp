#include "natalie.hpp"

#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

namespace Natalie {

namespace globals {

    static Optional<long> maxgroups;

}

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

Value ProcessModule::kill(Env *env, Args &&args) {
    env->ensure_no_extra_keywords(args.pop_keyword_hash());
    args.ensure_argc_at_least(env, 2);
    auto signal = args.shift();
    auto pids = args.to_array();
    nat_int_t signo;
    bool pid_contains_self = false;

    if (signal.is_symbol())
        signal = signal->to_s(env);
    if (signal.is_integer()) {
        signo = IntegerObject::convert_to_nat_int_t(env, signal);
    } else if (signal.is_string() || signal->respond_to(env, "to_str"_s)) {
        auto signame = signal.to_str(env)->delete_prefix(env, new StringObject { "SIG" });
        auto signo_val = SignalModule::list(env)->as_hash()->ref(env, signame);
        if (signo_val.is_nil())
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

long ProcessModule::maxgroups() {
    if (!globals::maxgroups)
        globals::maxgroups = sysconf(_SC_NGROUPS_MAX);
    return *globals::maxgroups;
}

Value ProcessModule::setmaxgroups(Env *env, Value val) {
    Value int_val = Object::to_int(env, val);
    if (int_val.send(env, "positive?"_s).is_falsey())
        env->raise("ArgumentError", "maxgroups {} should be positive", int_val->inspect_str(env));
    const long actual_maxgroups = sysconf(_SC_NGROUPS_MAX);
    globals::maxgroups = std::min(IntegerObject::convert_to_native_type<long>(env, int_val), actual_maxgroups);
    return val;
}

Value ProcessModule::times(Env *env) {
    rusage rusage_self, rusage_children;
    if (getrusage(RUSAGE_SELF, &rusage_self) == -1)
        env->raise_errno();
    if (getrusage(RUSAGE_CHILDREN, &rusage_children) == -1)
        env->raise_errno();
    auto tv_to_float = [](const timeval tv) {
        return new FloatObject { static_cast<double>(tv.tv_sec) + static_cast<double>(tv.tv_usec) / 1e6 };
    };
    auto utime = tv_to_float(rusage_self.ru_utime);
    auto stime = tv_to_float(rusage_self.ru_stime);
    auto cutime = tv_to_float(rusage_children.ru_utime);
    auto cstime = tv_to_float(rusage_children.ru_stime);
    auto Tms = fetch_nested_const({ "Process"_s, "Tms"_s })->as_class();
    return _new(env, Tms, { utime, stime, cutime, cstime }, nullptr);
}

Value ProcessModule::wait(Env *env, Value pidval, Value flagsval) {
    const pid_t pid = pidval ? IntegerObject::convert_to_native_type<pid_t>(env, pidval) : -1;
    const int flags = (flagsval && !flagsval.is_nil()) ? IntegerObject::convert_to_native_type<int>(env, flagsval) : 0;
    int status;
    const auto result = waitpid(pid, &status, flags);
    if (result == -1)
        env->raise_errno();
    set_status_object(env, result, status);
    return Value::integer(static_cast<nat_int_t>(result));
}

}
