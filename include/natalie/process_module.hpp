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
