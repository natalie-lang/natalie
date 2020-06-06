#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *File_initialize(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    IoValue *self = self_value->as_io();
    NAT_ASSERT_ARGC2(1, 2); // TODO: Ruby accepts 3 args??
    Value *filename = args[0];
    NAT_ASSERT_TYPE(filename, Value::Type::String, "String");
    int flags = O_RDONLY;
    if (argc > 1) {
        Value *flags_obj = args[1];
        switch (NAT_TYPE(flags_obj)) {
        case Value::Type::Integer:
            flags = NAT_INT_VALUE(flags_obj);
            break;
        case Value::Type::String: {
            const char *flags_str = flags_obj->as_string()->str;
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
                NAT_RAISE(env, "ArgumentError", "invalid access mode %s", flags_str);
            }
            break;
        }
        default:
            NAT_RAISE(env, "TypeError", "no implicit conversion of %s into String", NAT_OBJ_CLASS(flags_obj)->class_name);
        }
    }
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fileno = open(filename->as_string()->str, flags, mode);
    if (fileno == -1) {
        Value *exception_args[2] = { filename, integer(env, errno) };
        ExceptionValue *error = send(env, const_get(env, NAT_OBJECT, "SystemCallError", true), "exception", 2, exception_args, NULL)->as_exception();
        env->raise_exception(error);
        abort();
    } else {
        self->fileno = fileno;
        return self;
    }
}

Value *File_expand_path(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC2(1, 2);
    Value *path = args[0];
    NAT_ASSERT_TYPE(path, Value::Type::String, "String");
    if (path->as_string()->str_len > 0 && path->as_string()->str[0] == '/') {
        return path;
    }
    Value *merged;
    if (argc == 2) {
        Value *root = File_expand_path(env, self, 1, args + 1, NULL);
        merged = sprintf(env, "%S/%S", root, path);
    } else {
        char root[4096];
        if (getcwd(root, 4096)) {
            merged = sprintf(env, "%s/%S", root, path);
        } else {
            NAT_RAISE(env, "RuntimeError", "could not get current directory");
        }
    }
    RegexpValue *dotdot = regexp_new(env, "[^/]*/\\.\\./");
    Value *empty_string = string(env, "");
    while (Regexp_match(env, dotdot, 1, &merged, nullptr)->is_truthy()) {
        Value *args[2] = { dotdot, empty_string };
        merged = String_sub(env, merged, 2, args, NULL);
    }
    return merged;
}

}
