#pragma once

#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include "natalie/symbol_object.hpp"
#include <sys/stat.h>

// major/minor macro location
#ifdef __linux__
#include <sys/sysmacros.h>
#else
#include <sys/types.h>
#endif

#if defined(__APPLE__)
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#define st_mtim st_mtimespec
#endif

namespace Natalie {

class FileStatObject : public Object {
public:
    FileStatObject()
        : Object { Object::Type::FileStat,
            GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class()->const_fetch("Stat"_s)->as_class() } { }
    FileStatObject(ClassObject *klass)
        : Object { Object::Type::FileStat, klass } { }

    FileStatObject(struct stat status)
        : Object { Object::Type::FileStat,
            GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class()->const_fetch("Stat"_s)->as_class() } {
        fstatus = status;
    }

    Value initialize(Env *, Value);
    bool is_blockdev() const;
    bool is_chardev() const;
    bool is_directory() const;
    bool is_pipe() const;
    bool is_file() const;
    bool is_owned() const;
    Value is_size() const;
    bool is_setgid() const;
    bool is_setuid() const;
    bool is_socket() const;
    bool is_sticky() const;
    bool is_symlink() const;
    Value world_readable() const;
    Value world_writable() const;
    bool is_zero() const;

    Value atime(Env *) const;
    Value birthtime(Env *) const;
    Value comparison(Env *, Value) const;
    Value ctime(Env *) const;
    Value mtime(Env *) const;
    Value blksize() const;
    Value blocks() const;
    Value dev() const;
    Value dev_major() const;
    Value dev_minor() const;
    Value ftype() const;
    Value ino() const;
    Value nlink() const;
    Value mode() const;
    Value rdev() const;
    Value rdev_major() const;
    Value rdev_minor() const;
    Value size() const;
    Value uid() const;
    Value gid() const;

private:
    struct stat fstatus;
};

}
