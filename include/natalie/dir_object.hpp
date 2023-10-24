#pragma once

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "natalie/forward.hpp"
#include "natalie/integer_object.hpp"
#include "natalie/nil_object.hpp"
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

    virtual ~DirObject();

    virtual void visit_children(Visitor &visitor) override {
        Object::visit_children(visitor);
        visitor.visit(m_path);
        visitor.visit(m_encoding);
    }

    static Value size_fn(Env *env, Value self, Args, Block *) {
        return Value(NilObject::the());
    }

    Value children(Env *env); // for internal use
    Value close(Env *env);
    Value each(Env *env, Block *block);
    Value each_child(Env *env, Block *block);
    Value entries(Env *env); // for internal use
    int fileno(Env *env);
    Value initialize(Env *env, Value path, Value encoding);
    Value read(Env *env);
    Value rewind(Env *env);
    Value seek(Env *env, Value position);
    nat_int_t set_pos(Env *env, Value position);
    Value tell(Env *env);

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

    static Value children(Env *env, Value path, Value encoding);
    static Value entries(Env *env, Value path, Value encoding);
    static Value each_child(Env *env, Value path, Value encoding, Block *block);
    static Value foreach (Env *env, Value path, Value encoding, Block * block);

private:
    DIR *m_dir { nullptr };
    StringObject *m_path { nullptr };
    EncodingObject *m_encoding { nullptr };
};

}
