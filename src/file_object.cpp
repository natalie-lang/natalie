#include "natalie.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>

namespace Natalie {

Value FileObject::initialize(Env *env, Value filename, Value flags_obj, Block *block) {
    filename->assert_type(env, Object::Type::String, "String");
    int flags = O_RDONLY;
    if (flags_obj) {
        switch (flags_obj->type()) {
        case Object::Type::Integer:
            flags = flags_obj->as_integer()->to_nat_int_t();
            break;
        case Object::Type::String: {
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
                env->raise("ArgumentError", "invalid access mode {}", flags_str);
            }
            break;
        }
        default:
            env->raise("TypeError", "no implicit conversion of {} into String", flags_obj->klass()->inspect_str());
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

Value FileObject::expand_path(Env *env, Value path, Value root) {
    path->assert_type(env, Object::Type::String, "String");
    StringObject *merged;
    if (path->as_string()->length() > 0 && path->as_string()->c_str()[0] == '/') {
        merged = path->as_string();
    } else if (root) {
        root->assert_type(env, Object::Type::String, "String");
        root = expand_path(env, root, nullptr);
        merged = StringObject::format(env, "{}/{}", root->as_string(), path->as_string());
    } else {
        char root[MAXPATHLEN + 1];
        if (!getcwd(root, MAXPATHLEN + 1))
            env->raise_errno();
        merged = StringObject::format(env, "{}/{}", root, path->as_string());
    }
    // collapse ..
    RegexpObject dotdot { env, "[^/]*/\\.\\.(/|\\z)" };
    StringObject empty_string { "" };
    do {
        merged = merged->sub(env, &dotdot, &empty_string)->as_string();
    } while (env->has_last_match());
    // collapse .
    RegexpObject dot { env, "/\\.(/|\\z)" };
    StringObject slash { "/" };
    do {
        merged = merged->sub(env, &dot, &slash)->as_string();
    } while (env->has_last_match());
    // remove trailing slash
    if (merged->length() > 1 && merged->c_str()[merged->length() - 1] == '/') {
        merged->truncate(merged->length() - 1);
    }
    return merged;
}

Value FileObject::unlink(Env *env, Value path) {
    int result = ::unlink(path->as_string()->c_str());
    if (result == 0) {
        return Value::integer(1);
    } else {
        auto SystemCallError = GlobalEnv::the()->Object()->const_fetch("SystemCallError"_s);
        auto exception = SystemCallError.send(env, "exception"_s, { Value::integer(errno) })->as_exception();
        env->raise_exception(exception);
    }
}

void FileObject::build_constants(Env *env, ClassObject *klass) {
    Value Constants = new ModuleObject { "Constants" };
    klass->const_set("Constants"_s, Constants);
    klass->const_set("APPEND"_s, Value::integer(O_APPEND));
    Constants->const_set("APPEND"_s, Value::integer(O_APPEND));
    klass->const_set("RDONLY"_s, Value::integer(O_RDONLY));
    Constants->const_set("RDONLY"_s, Value::integer(O_RDONLY));
    klass->const_set("WRONLY"_s, Value::integer(O_WRONLY));
    Constants->const_set("WRONLY"_s, Value::integer(O_WRONLY));
    klass->const_set("TRUNC"_s, Value::integer(O_TRUNC));
    Constants->const_set("TRUNC"_s, Value::integer(O_TRUNC));
    klass->const_set("CREAT"_s, Value::integer(O_CREAT));
    Constants->const_set("CREAT"_s, Value::integer(O_CREAT));
    klass->const_set("DSYNC"_s, Value::integer(O_DSYNC));
    Constants->const_set("DSYNC"_s, Value::integer(O_DSYNC));
    klass->const_set("EXCL"_s, Value::integer(O_EXCL));
    Constants->const_set("EXCL"_s, Value::integer(O_EXCL));
    klass->const_set("NOCTTY"_s, Value::integer(O_NOCTTY));
    Constants->const_set("NOCTTY"_s, Value::integer(O_NOCTTY));
    klass->const_set("NOFOLLOW"_s, Value::integer(O_NOFOLLOW));
    Constants->const_set("NOFOLLOW"_s, Value::integer(O_NOFOLLOW));
    klass->const_set("NONBLOCK"_s, Value::integer(O_NONBLOCK));
    Constants->const_set("NONBLOCK"_s, Value::integer(O_NONBLOCK));
    klass->const_set("RDWR"_s, Value::integer(O_RDWR));
    Constants->const_set("RDWR"_s, Value::integer(O_RDWR));
    klass->const_set("SYNC"_s, Value::integer(O_SYNC));
    Constants->const_set("SYNC"_s, Value::integer(O_SYNC));
}

bool FileObject::file(Env *env, Value path) {
    struct stat sb;
    path->assert_type(env, Object::Type::String, "String");
    if (stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISREG(sb.st_mode);
}

bool FileObject::directory(Env *env, Value path) {
    struct stat sb;
    path->assert_type(env, Object::Type::String, "String");
    if (stat(path->as_string()->c_str(), &sb) == -1)
        return false;
    return S_ISDIR(sb.st_mode);
}

}
