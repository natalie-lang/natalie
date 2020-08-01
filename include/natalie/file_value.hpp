#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_value.hpp"
#include "natalie/io_value.hpp"
#include "natalie/regexp_value.hpp"
#include "natalie/string_value.hpp"

namespace Natalie {

struct FileValue : IoValue {
    FileValue(Env *env)
        : IoValue { env } { }

    Value *initialize(Env *, Value *, Value *, Block *);

    static Value *expand_path(Env *env, Value *path, Value *root) {
        NAT_ASSERT_TYPE(path, Value::Type::String, "String");
        if (path->as_string()->length() > 0 && path->as_string()->c_str()[0] == '/') {
            return path;
        }
        StringValue *merged;
        if (root) {
            root = expand_path(env, root, nullptr);
            merged = StringValue::sprintf(env, "%S/%S", root, path);
        } else {
            char root[4096];
            if (getcwd(root, 4096)) {
                merged = StringValue::sprintf(env, "%s/%S", root, path);
            } else {
                NAT_RAISE(env, "RuntimeError", "could not get current directory");
            }
        }
        // collapse ..
        RegexpValue dotdot { env, "[^/]*/\\.\\.(/|\\z)" };
        StringValue empty_string { env, "" };
        do {
            merged = merged->sub(env, &dotdot, &empty_string);
        } while (env->caller()->match());
        // remove trailing slash
        if (merged->length() > 1 && merged->c_str()[merged->length() - 1] == '/') {
            merged->truncate(merged->length() - 1);
        }
        return merged;
    }

    static void build_constants(Env *env, ClassValue *klass) {
        Value *Constants = new ModuleValue { env, "Constants" };
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
};

}
