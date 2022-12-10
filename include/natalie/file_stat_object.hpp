#pragma once

//#include <assert.h>
//#include <fcntl.h>
//#include <sys/stat.h>
#include <filesystem>
//#include "natalie/forward.hpp"
//#include "natalie/integer_object.hpp"
//#include "natalie/io_object.hpp"
//#include "natalie/regexp_object.hpp"
//#include "natalie/string_object.hpp"
//#include "natalie/symbol_object.hpp"
//#include "tm/defer.hpp"

namespace Natalie {

class FileStatObject : public Object {
public:
    FileStatObject(const StringObject &other)
        : Object { Object::Type::FileStat,
            GlobalEnv::the()->Object()->const_fetch("File"_s)->as_class()->const_fetch("Stat"_s)->as_class() } { }

    Value initialize(Env *, Value);
    Value ftype();

private:
    std::filesystem::file_status fstatus;
};

}
