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

namespace fileutil {
    // Utility Function Common to File and Dir
    Value convert_using_to_path(Env *env, Value path);
}

class FileObject : public IoObject {
public:
    FileObject()
        : IoObject { GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class() } { }

    Value initialize(Env *, Value, Value, Block *);

    static Value open(Env *env, Value filename, Value flags_obj, Block *block) {
        Value args[] = { filename, flags_obj };
        auto args_struct = Args { static_cast<size_t>(flags_obj ? 2 : 1), args };
        auto obj = _new(env, GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class(), args_struct, nullptr);
        if (block) {
            Defer close_file([&]() {
                obj->as_file()->close(env);
            });
            Value block_args[] = { obj };
            Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, block_args), nullptr);
            return result;
        } else {
            return obj;
        }
    }

    static Value expand_path(Env *env, Value path, Value root);
    static void unlink(Env *env, Value path);
    static Value unlink(Env *env, Args args);

    static void build_constants(Env *env, ClassObject *klass);

    static bool exist(Env *env, Value path);

    static bool is_blockdev(Env *env, Value path);
    static bool is_chardev(Env *env, Value path);
    static bool is_directory(Env *env, Value path);
    static bool is_executable(Env *env, Value path);
    static bool is_executable_real(Env *env, Value path);
    static bool is_file(Env *env, Value path);
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

    static Value umask(Env *env, Value mask);
    static Value ftype(Env *env, Value path);
    static Value size(Env *env, Value path);
    static Value world_readable(Env *env, Value path);
    static Value world_writable(Env *env, Value path);

    static Value link(Env *env, Value from, Value to);
    static Value symlink(Env *env, Value from, Value to);
    static Value mkfifo(Env *env, Value path, Value mode);
    static Value chmod(Env *env, Value mode, Value path);
    Value chmod(Env *env, Value mode);

    static Value lstat(Env *env, Value path);
    static Value stat(Env *env, Value path);

    Value lstat(Env *env); // instance method
    Value stat(Env *env); // instance method

    String path() const { return m_path; }
    void set_path(String path) { m_path = path; };

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<FileObject %p>", this);
    }

private:
    String m_path {};
};

}
