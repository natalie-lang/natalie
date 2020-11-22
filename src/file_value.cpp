#include "natalie.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace Natalie {

Value *FileValue::initialize(Env *env, Value *filename, Value *flags_obj, Block *block) {
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
        Value *exception_args[2] = { filename, new IntegerValue { env, errno } };
        ExceptionValue *error = env->Object()->const_find(env, "SystemCallError")->send(env, "exception", 2, exception_args, nullptr)->as_exception();
        env->raise_exception(error);
    } else {
        set_fileno(fileno);
        return this;
    }
}

}
