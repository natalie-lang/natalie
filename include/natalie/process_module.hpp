#pragma once

#include <grp.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include "natalie/forward.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/object.hpp"

namespace Natalie {

class ProcessModule : public Object {
public:
    static Value egid(Env *env) {
        gid_t egid = getegid();
        return Value::integer(egid);
    }
    static Value euid(Env *env) {
        uid_t euid = geteuid();
        return Value::integer(euid);
    }
    static Value gid(Env *env) {
        gid_t gid = getgid();
        return Value::integer(gid);
    }
    static Value pid(Env *env) {
        pid_t pid = getpid();
        return Value::integer(pid);
    }
    static Value ppid(Env *env) {
        pid_t pid = getppid();
        return Value::integer(pid);
    }
    static Value uid(Env *env) {
        uid_t uid = getuid();
        return Value::integer(uid);
    }

    static Value setuid(Env *env, Value idval) {
        uid_t uid = value_to_uid(env, idval);
        if (setreuid(uid, -1) < 0)
            env->raise_errno();
        return idval;
    }

    static Value seteuid(Env *env, Value idval) {
        uid_t euid = value_to_uid(env, idval);
        if (setreuid(-1, euid) < 0)
            env->raise_errno();
        return idval;
    }

    static Value setgid(Env *env, Value idval) {
        gid_t gid = value_to_gid(env, idval);
        if (setregid(gid, -1) < 0)
            env->raise_errno();
        return idval;
    }

    static Value setegid(Env *env, Value idval) {
        gid_t egid = value_to_gid(env, idval);
        if (setregid(-1, egid) < 0)
            env->raise_errno();
        return idval;
    }

    static int getpgid(Env *env, Value idval) {
        pid_t idnum = IntegerObject::convert_to_nat_int_t(env, idval);
        pid_t pgrp = ::getpgid(idnum);
        if (pgrp < 0) env->raise_errno();
        return pgrp;
    }

    static int getpgrp(Env *env) {
        pid_t pgrp = ::getpgrp();
        return pgrp;
    }

    static int setpgrp(Env *env) {
        if (::setpgid(0, 0) < 0) env->raise_errno();
        return 0;
    }

    static int getpriority(Env *env, Value which, Value who) {
        int whichnum = IntegerObject::convert_to_nat_int_t(env, which);
        id_t whonum = IntegerObject::convert_to_nat_int_t(env, who);
        errno = 0;
        int pri = ::getpriority(whichnum, whonum);
        if (errno) env->raise_errno();
        return pri;
    }

    static Value getrlimit(Env *env, Value resource) {
        struct rlimit rlim;
        int resrc = value_to_resource(env, resource);
        auto result = ::getrlimit(resrc, &rlim);
        if (result < 0) env->raise_errno();
        auto curlim = new IntegerObject { (nat_int_t)(rlim.rlim_cur) };
        auto maxlim = new IntegerObject { (nat_int_t)(rlim.rlim_max) };
        return new ArrayObject { curlim, maxlim };
    }

    static int getsid(Env *env, Value pid) {
        pid_t pidnum;
        if (!pid || pid->is_nil()) {
            pidnum = 0;
        } else {
            pidnum = IntegerObject::convert_to_nat_int_t(env, pid);
        }
        pid_t sid = ::getsid(pidnum);
        if (sid < 0) env->raise_errno();
        return sid;
    }

    static Value clock_gettime(Env *, Value);
    static Value groups(Env *env);

private:
    static uid_t value_to_uid(Env *env, Value idval) {
        uid_t uid;
        if (idval->is_string()) {
            struct passwd *pass;
            pass = getpwnam(idval->as_string()->c_str());
            if (pass == NULL)
                env->raise("ArgumentError", "can't find user {}", idval->as_string()->c_str());
            uid = pass->pw_uid;
        } else {
            idval->assert_type(env, Object::Type::Integer, "Integer");
            uid = idval->as_integer()->to_nat_int_t();
        }
        return uid;
    }

    static gid_t value_to_gid(Env *env, Value idval) {
        gid_t gid;
        if (idval->is_string()) {
            auto grp = getgrnam(idval->as_string()->c_str());
            if (grp == NULL)
                env->raise("ArgumentError", "can't find group {}", idval->as_string()->c_str());
            gid = grp->gr_gid;
        } else {
            idval->assert_type(env, Object::Type::Integer, "Integer");
            gid = idval->as_integer()->to_nat_int_t();
        }
        return gid;
    }

    static int value_to_resource(Env *env, Value val) {
        int resource;
        auto to_str = "to_str"_s;
        SymbolObject *rlimit_symbol = nullptr;
        if (val->is_symbol()) {
            rlimit_symbol = val->as_symbol();
        } else if (val->is_string()) {
            rlimit_symbol = val->as_string()->to_symbol(env);
        } else if (val->respond_to(env, to_str)) {
            // Need to support nil, don't use Object::to_str
            auto tsval = val->send(env, to_str);
            if (tsval->is_string()) {
                rlimit_symbol = tsval->as_string()->to_sym(env)->as_symbol();
            }
        }
        if (rlimit_symbol) {
            rlimit_symbol->assert_type(env, Object::Type::Symbol, "Symbol");
            StringObject *rlimit_str = new StringObject { "RLIMIT_" };
            rlimit_str->append(rlimit_symbol->string());
            rlimit_symbol = rlimit_str->as_string()->to_symbol(env);
            auto ProcessMod = GlobalEnv::the()->Object()->const_fetch("Process"_s)->as_module();
            Value rlimval = ProcessMod->const_get(rlimit_symbol);
            if (!rlimval || !rlimval->is_integer()) {
                env->raise("ArgumentError", "invalid resource {}", rlimit_symbol->string());
            }
            val = rlimval->as_integer();
        }

        resource = IntegerObject::convert_to_nat_int_t(env, val);
        return resource;
    }
};

}
