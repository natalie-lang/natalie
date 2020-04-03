#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "builtin.h"
#include "gc.h"
#include "natalie.h"

NatObject *File_initialize(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC2(1, 2); // TODO: Ruby accepts 3 args??
    NatObject *filename = args[0];
    NAT_ASSERT_TYPE(filename, NAT_VALUE_STRING, "String");
    int flags = O_RDONLY;
    if (argc > 1) {
        NatObject *flags_obj = args[1];
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
                NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "ArgumentError"), "invalid access mode %s", flags_obj->str);
            }
            break;
        default:
            NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "TypeError"), "no implicit conversion of %s into String", flags_obj->klass->class_name);
        }
    }
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fileno = open(filename->str, flags, mode);
    if (fileno == -1) {
        NatObject **exception_args = calloc(2, sizeof(NatObject *));
        exception_args[0] = filename;
        exception_args[1] = nat_integer(env, errno);
        NatObject *error = nat_send(env, nat_const_get(env, NAT_OBJECT, "SystemCallError"), "exception", 2, exception_args, NULL);
        nat_raise_exception(env, error);
        abort();
    } else {
        self->fileno = fileno;
        return self;
    }
}
