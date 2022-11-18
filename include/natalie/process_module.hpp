#pragma once

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
    static void build_constants(Env *env, ModuleObject *klass) {
#ifdef PRIO_PGRP
        klass->const_set("PRIO_PGRP"_s, Value::integer(PRIO_PGRP));
#endif
#ifdef PRIO_PROCESS
        klass->const_set("PRIO_PROCESS"_s, Value::integer(PRIO_PROCESS));
#endif
#ifdef PRIO_USER
        klass->const_set("PRIO_USER"_s, Value::integer(PRIO_USER));
#endif

#ifdef RLIMIT_AS
        klass->const_set("RLIMIT_AS"_s, Value::integer(RLIMIT_AS));
#endif
#ifdef RLIMIT_CORE
        klass->const_set("RLIMIT_CORE"_s, Value::integer(RLIMIT_CORE));
#endif
#ifdef RLIMIT_CPU
        klass->const_set("RLIMIT_CPU"_s, Value::integer(RLIMIT_CPU));
#endif
#ifdef RLIMIT_DATA
        klass->const_set("RLIMIT_DATA"_s, Value::integer(RLIMIT_DATA));
#endif
#ifdef RLIMIT_FSIZE
        klass->const_set("RLIMIT_FSIZE"_s, Value::integer(RLIMIT_FSIZE));
#endif
#ifdef RLIMIT_MEMLOCK
        klass->const_set("RLIMIT_MEMLOCK"_s, Value::integer(RLIMIT_MEMLOCK));
#endif
#ifdef RLIMIT_MSGQUEUE
        klass->const_set("RLIMIT_MSGQUEUE"_s, Value::integer(RLIMIT_MSGQUEUE));
#endif
#ifdef RLIMIT_NICE
        klass->const_set("RLIMIT_NICE"_s, Value::integer(RLIMIT_NICE));
#endif
#ifdef RLIMIT_NOFILE
        klass->const_set("RLIMIT_NOFILE"_s, Value::integer(RLIMIT_NOFILE));
#endif
#ifdef RLIMIT_NPROC
        klass->const_set("RLIMIT_NPROC"_s, Value::integer(RLIMIT_NPROC));
#endif
#ifdef RLIMIT_NPTS
        klass->const_set("RLIMIT_NPTS"_s, Value::integer(RLIMIT_NPTS));
#endif
#ifdef RLIMIT_RSS
        klass->const_set("RLIMIT_RSS"_s, Value::integer(RLIMIT_RSS));
#endif
#ifdef RLIMIT_RTPRIO
        klass->const_set("RLIMIT_RTPRIO"_s, Value::integer(RLIMIT_RTPRIO));
#endif
#ifdef RLIMIT_RTTIME
        klass->const_set("RLIMIT_RTTIME"_s, Value::integer(RLIMIT_RTTIME));
#endif
#ifdef RLIMIT_SBSIZE
        klass->const_set("RLIMIT_SBSIZE"_s, Value::integer(RLIMIT_SBSIZE));
#endif
#ifdef RLIMIT_SIGPENDING
        klass->const_set("RLIMIT_SIGPENDING"_s, Value::integer(RLIMIT_SIGPENDING));
#endif
#ifdef RLIMIT_STACK
        klass->const_set("RLIMIT_STACK"_s, Value::integer(RLIMIT_STACK));
#endif
#ifdef RLIM_INFINITY
        klass->const_set("RLIM_INFINITY"_s, IntegerObject::create(Integer(BigInt(RLIM_INFINITY))));
#endif
#ifdef RLIM_SAVED_CUR
        klass->const_set("RLIM_SAVED_CUR"_s, IntegerObject::create(Integer(BigInt(RLIM_SAVED_CUR))));
#endif
#ifdef RLIM_SAVED_MAX
        klass->const_set("RLIM_SAVED_MAX"_s, IntegerObject::create(Integer(BigInt(RLIM_SAVED_MAX))));
#endif
#ifdef WNOHANG
        klass->const_set("WNOHANG"_s, Value::integer(WNOHANG));
#endif
#ifdef WUNTRACED
        klass->const_set("WUNTRACED"_s, Value::integer(WUNTRACED));
#endif
    }

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
        if (setresuid(uid, -1, -1) < 0)
            env->raise_errno();
        return idval;
    }

    static Value seteuid(Env *env, Value idval) {
        uid_t euid = value_to_uid(env, idval);
        if (setresuid(-1, euid, -1) < 0)
            env->raise_errno();
        return idval;
    }

    static Value setgid(Env *env, Value idval) {
        gid_t gid = value_to_gid(env, idval);
        if (setresgid(gid, -1, -1) < 0)
            env->raise_errno();
        return idval;
    }

    static Value setegid(Env *env, Value idval) {
        gid_t egid = value_to_gid(env, idval);
        if (setresgid(-1, egid, -1) < 0)
            env->raise_errno();
        return idval;
    }

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
            struct passwd *pass;
            pass = getpwnam(idval->as_string()->c_str());
            if (pass == NULL)
                env->raise("ArgumentError", "can't find user {}", idval->as_string()->c_str());
            gid = pass->pw_gid;
        } else {
            idval->assert_type(env, Object::Type::Integer, "Integer");
            gid = idval->as_integer()->to_nat_int_t();
        }
        return gid;
    }
};

}
