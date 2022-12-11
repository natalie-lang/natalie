#pragma once

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "natalie/forward.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"
#include "tm/defer.hpp"

namespace Natalie {

class DirObject : public Object {

public:
    DirObject()
        : Object { GlobalEnv::the()->Object()->const_fetch("Dir"_s)->as_class() } { }

    static bool is_empty(Env *, Value);
    static Value home(Env *, Value);
    static Value mkdir(Env *env, Value path, Value mode);
    //static Value pwd(Env *env);
    //static Value chdir(Env *env, Value path) // also takes a block
    static Value rmdir(Env *env, Value path); // same as .delete, .unlink
};

}
