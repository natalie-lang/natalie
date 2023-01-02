#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include <sys/stat.h>

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
    Value ftype() const;
    bool is_blockdev() const;
    bool is_chardev() const;
    bool is_directory() const;
    bool is_pipe() const;
    bool is_file() const;
    Value is_size() const;
    bool is_symlink() const;
    bool is_zero() const;
    Value size() const;

private:
    struct stat fstatus;
};

}
