#pragma once

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

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
        : Object { Object::Type::Dir, GlobalEnv::the()->Object()->const_fetch("Dir"_s)->as_class() } { }
    DirObject(ClassObject *klass)
        : Object { Object::Type::Dir, klass } { }

    Value close(Env *env);
    Value read(Env *env);
    Value tell(Env *env);
    Value rewind(Env *env);
    Value seek(Env *env, Value position);

    Value fileno(Env *env);
    Value initialize(Env *env, Value path, Value encoding);
    StringObject *path(Env *env) { return m_path; }

    StringObject *inspect(Env *env);

    static bool is_empty(Env *, Value);
    static Value chdir(Env *env, Value path, Block *block);
    static Value chroot(Env *env, Value path);
    static Value home(Env *, Value);
    static Value mkdir(Env *env, Value path, Value mode);
    static Value open(Env *env, Value path, Value encoding, Block *block);
    static Value pwd(Env *env);
    static Value rmdir(Env *env, Value path); // same as .delete, .unlink

private:
    DIR *m_dir { nullptr };
    StringObject *m_path { nullptr };
};

}
