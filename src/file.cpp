#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *File_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC2(1, 2); // TODO: Ruby accepts 3 args??
    Value *filename = args[0];
    NAT_ASSERT_TYPE(filename, NAT_VALUE_STRING, "String");
    int flags = O_RDONLY;
    if (argc > 1) {
        Value *flags_obj = args[1];
        switch (NAT_TYPE(flags_obj)) {
        case NAT_VALUE_INTEGER:
            flags = NAT_INT_VALUE(flags_obj);
            break;
        case NAT_VALUE_STRING:
            if (strcmp(flags_obj->str, "r") == 0) {
                flags = O_RDONLY;
            } else if (strcmp(flags_obj->str, "r+") == 0) {
                flags = O_RDWR;
            } else if (strcmp(flags_obj->str, "w") == 0) {
                flags = O_WRONLY | O_CREAT | O_TRUNC;
            } else if (strcmp(flags_obj->str, "w+") == 0) {
                flags = O_RDWR | O_CREAT | O_TRUNC;
            } else if (strcmp(flags_obj->str, "a") == 0) {
                flags = O_WRONLY | O_CREAT | O_APPEND;
            } else if (strcmp(flags_obj->str, "a+") == 0) {
                flags = O_RDWR | O_CREAT | O_APPEND;
            } else {
                NAT_RAISE(env, "ArgumentError", "invalid access mode %s", flags_obj->str);
            }
            break;
        default:
            NAT_RAISE(env, "TypeError", "no implicit conversion of %s into String", NAT_OBJ_CLASS(flags_obj)->class_name);
        }
    }
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fileno = open(filename->str, flags, mode);
    if (fileno == -1) {
        Value *exception_args[2] = { filename, integer(env, errno) };
        Value *error = send(env, const_get(env, NAT_OBJECT, "SystemCallError", true), "exception", 2, exception_args, NULL);
        raise_exception(env, error);
        abort();
    } else {
        self->fileno = fileno;
        return self;
    }
}

Value *File_expand_path(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC2(1, 2);
    Value *path = args[0];
    NAT_ASSERT_TYPE(path, NAT_VALUE_STRING, "String");
    if (path->str_len > 0 && path->str[0] == '/') {
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
    Value *dotdot = regexp(env, "[^/]*/\\.\\./");
    Value *empty_string = string(env, "");
    while (truthy(Regexp_match(env, dotdot, 1, &merged, NULL))) {
        Value *args[2] = { dotdot, empty_string };
        merged = String_sub(env, merged, 2, args, NULL);
    }
    return merged;
}

}
