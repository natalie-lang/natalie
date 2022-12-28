#pragma once

#include "natalie/forward.hpp"
#include "natalie/object.hpp"
#include <filesystem>

namespace Natalie {

class FileStatObject : public Object {
public:
    FileStatObject()
        : Object { Object::Type::FileStat,
                   GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class()->const_fetch("Stat"_s)->as_class() } { }
    FileStatObject(ClassObject *klass)
        : Object { Object::Type::FileStat, klass } { }
    
    FileStatObject(const std::filesystem::file_status status)
        : Object { Object::Type::FileStat,
            GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class()->const_fetch("Stat"_s)->as_class() } {
        fstatus = status;
    }

    Value initialize(Env *, Value);
    Value ftype();

private:
    std::filesystem::file_status fstatus;
};

}
