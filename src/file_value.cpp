#include "natalie.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>

namespace Natalie {

ValuePtr FileValue::initialize(Env *env, ValuePtr filename, ValuePtr flags_obj, Block *block) {
    filename->assert_type(env, Value::Type::String, "String");
    int flags = O_RDONLY;
    if (flags_obj) {
        switch (flags_obj->type()) {
        case Value::Type::Integer:
            flags = flags_obj->as_integer()->to_nat_int_t();
            break;
        case Value::Type::String: {
            const char *flags_str = flags_obj->as_string()->c_str();
            if (strcmp(flags_str, "r") == 0) {
                flags = O_RDONLY;
            } else if (strcmp(flags_str, "r+") == 0) {
                flags = O_RDWR;
            } else if (strcmp(flags_str, "w") == 0) {
                flags = O_WRONLY | O_CREAT | O_TRUNC;
            } else if (strcmp(flags_str, "w+") == 0) {
                flags = O_RDWR | O_CREAT | O_TRUNC;
            } else if (strcmp(flags_str, "a") == 0) {
                flags = O_WRONLY | O_CREAT | O_APPEND;
            } else if (strcmp(flags_str, "a+") == 0) {
                flags = O_RDWR | O_CREAT | O_APPEND;
            } else {
                env->raise("ArgumentError", "invalid access mode %s", flags_str);
            }
            break;
        }
        default:
            env->raise("TypeError", "no implicit conversion of %s into String", flags_obj->klass()->class_name());
        }
    }
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fileno = ::open(filename->as_string()->c_str(), flags, mode);
    if (fileno == -1) {
        env->raise_errno();
    } else {
        set_fileno(fileno);
        return this;
    }
}

ValuePtr FileValue::expand_path(Env *env, ValuePtr path, ValuePtr root) {
    path->assert_type(env, Value::Type::String, "String");
    StringValue *merged;
    if (path->as_string()->length() > 0 && path->as_string()->c_str()[0] == '/') {
        merged = path->as_string();
    } else if (root) {
        root = expand_path(env, root, nullptr);
        merged = StringValue::sprintf(env, "%S/%S", root, path);
    } else {
        char root[MAXPATHLEN + 1];
        if (!getcwd(root, MAXPATHLEN + 1))
            env->raise_errno();
        merged = StringValue::sprintf(env, "%s/%S", root, path);
    }
    // collapse ..
    RegexpValue dotdot { env, "[^/]*/\\.\\.(/|\\z)" };
    StringValue empty_string { env, "" };
    do {
        merged = merged->sub(env, &dotdot, &empty_string)->as_string();
    } while (env->caller()->match());
    // collapse .
    RegexpValue dot { env, "/\\.(/|\\z)" };
    StringValue slash { env, "/" };
    do {
        merged = merged->sub(env, &dot, &slash)->as_string();
    } while (env->caller()->match());
    // remove trailing slash
    if (merged->length() > 1 && merged->c_str()[merged->length() - 1] == '/') {
        merged->truncate(merged->length() - 1);
    }
    return merged;
}

ValuePtr FileValue::unlink(Env *env, ValuePtr path) {
    int result = ::unlink(path->as_string()->c_str());
    if (result == 0) {
        return new IntegerValue { env, 1 };
    } else {
        ValuePtr args[] = { new IntegerValue { env, errno } };
        auto exception = env->Object()->const_fetch("SystemCallError")->send(env, "exception", 1, args)->as_exception();
        env->raise_exception(exception);
    }
}

void FileValue::build_constants(Env *env, ClassValue *klass) {
    ValuePtr Constants = new ModuleValue { env, "Constants" };
    klass->const_set(env, "Constants", Constants);
    klass->const_set(env, "APPEND", new IntegerValue { env, O_APPEND });
    Constants->const_set(env, "APPEND", new IntegerValue { env, O_APPEND });
    klass->const_set(env, "RDONLY", new IntegerValue { env, O_RDONLY });
    Constants->const_set(env, "RDONLY", new IntegerValue { env, O_RDONLY });
    klass->const_set(env, "WRONLY", new IntegerValue { env, O_WRONLY });
    Constants->const_set(env, "WRONLY", new IntegerValue { env, O_WRONLY });
    klass->const_set(env, "TRUNC", new IntegerValue { env, O_TRUNC });
    Constants->const_set(env, "TRUNC", new IntegerValue { env, O_TRUNC });
    klass->const_set(env, "CREAT", new IntegerValue { env, O_CREAT });
    Constants->const_set(env, "CREAT", new IntegerValue { env, O_CREAT });
    klass->const_set(env, "DSYNC", new IntegerValue { env, O_DSYNC });
    Constants->const_set(env, "DSYNC", new IntegerValue { env, O_DSYNC });
    klass->const_set(env, "EXCL", new IntegerValue { env, O_EXCL });
    Constants->const_set(env, "EXCL", new IntegerValue { env, O_EXCL });
    klass->const_set(env, "NOCTTY", new IntegerValue { env, O_NOCTTY });
    Constants->const_set(env, "NOCTTY", new IntegerValue { env, O_NOCTTY });
    klass->const_set(env, "NOFOLLOW", new IntegerValue { env, O_NOFOLLOW });
    Constants->const_set(env, "NOFOLLOW", new IntegerValue { env, O_NOFOLLOW });
    klass->const_set(env, "NONBLOCK", new IntegerValue { env, O_NONBLOCK });
    Constants->const_set(env, "NONBLOCK", new IntegerValue { env, O_NONBLOCK });
    klass->const_set(env, "RDWR", new IntegerValue { env, O_RDWR });
    Constants->const_set(env, "RDWR", new IntegerValue { env, O_RDWR });
    klass->const_set(env, "SYNC", new IntegerValue { env, O_SYNC });
    Constants->const_set(env, "SYNC", new IntegerValue { env, O_SYNC });
}

}
