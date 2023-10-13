#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/io_object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"
#include "tm/defer.hpp"

namespace Natalie {

class FileObject : public IoObject {
public:
    FileObject()
        : IoObject { GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class() } { }

    Value initialize(Env *, Args, Block *);

    static Value expand_path(Env *env, Value path, Value root);
    static void unlink(Env *env, Value path);
    static Value unlink(Env *env, Args args);

    static void build_constants(Env *env, ModuleObject *);

    static bool exist(Env *env, Value path);

    static bool is_blockdev(Env *env, Value path);
    static bool is_chardev(Env *env, Value path);
    static bool is_directory(Env *env, Value path);
    static bool is_executable(Env *env, Value path);
    static bool is_executable_real(Env *env, Value path);
    static bool is_file(Env *env, Value path);
    static bool is_grpowned(Env *env, Value path);
    static bool is_owned(Env *env, Value path);
    static bool is_pipe(Env *env, Value path);
    static bool is_readable(Env *env, Value path);
    static bool is_readable_real(Env *env, Value path);
    static bool is_setgid(Env *env, Value path);
    static bool is_setuid(Env *env, Value path);
    static Value is_size(Env *env, Value path);
    static bool is_socket(Env *env, Value path);
    static bool is_sticky(Env *env, Value path);
    static bool is_symlink(Env *env, Value path);
    static bool is_writable(Env *env, Value path);
    static bool is_writable_real(Env *env, Value path);
    static bool is_zero(Env *env, Value path);

    static bool is_identical(Env *env, Value file1, Value file2);

    static Value atime(Env *env, Value path);
    static Value ctime(Env *env, Value path);
    static Value mtime(Env *env, Value path);
    static Value utime(Env *env, Args args);

    Value atime(Env *env);
    Value ctime(Env *env);
    Value mtime(Env *env);

    static Value umask(Env *env, Value mask);
    static Value ftype(Env *env, Value path);
    static Value size(Env *env, Value path);
    Value size(Env *env);
    static Value readlink(Env *, Value);
    static Value realpath(Env *, Value, Value);
    static Value world_readable(Env *env, Value path);
    static Value world_writable(Env *env, Value path);

    static nat_int_t link(Env *env, Value from, Value to);
    static nat_int_t rename(Env *env, Value from, Value to);
    static nat_int_t symlink(Env *env, Value from, Value to);
    static nat_int_t mkfifo(Env *env, Value path, Value mode);

    static Value chmod(Env *env, Args args);
    static Value chown(Env *env, Args args);
    Value chmod(Env *env, Value mode);
    Value chown(Env *env, Value uid, Value gid);

    static Value lstat(Env *env, Value path);
    static Value stat(Env *env, Value path);

    Value flock(Env *, Value);
    Value lstat(Env *env) const; // instance method

    static int truncate(Env *env, Value path, Value size);
    int truncate(Env *env, Value size) const; // instance method

    static StringObject *path(Env *env, Value path); // path class method

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<FileObject %p>", this);
    }
};

}
