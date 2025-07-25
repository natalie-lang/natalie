#pragma once

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "natalie/forward.hpp"
#include "natalie/integer_methods.hpp"
#include "natalie/object.hpp"
#include "natalie/regexp_object.hpp"
#include "natalie/string_object.hpp"
#include "natalie/symbol_object.hpp"
#include "tm/defer.hpp"

namespace Natalie {

class DirObject : public Object {

public:
    static DirObject *create() {
        return new DirObject();
    }

    static DirObject *create(ClassObject *klass) {
        return new DirObject(klass);
    }

    static DirObject *create(const DirObject &other) {
        return new DirObject(other);
    }

    virtual ~DirObject();

    virtual void visit_children(Visitor &visitor) const override {
        Object::visit_children(visitor);
        visitor.visit(m_path);
        visitor.visit(m_encoding);
    }

    static Value size_fn(Env *env, Value self, Args &&, Block *) {
        return Value::nil();
    }

    Value chdir_instance(Env *env, Block *block);
    Value children(Env *env); // for internal use
    Value close(Env *env);
    Value each(Env *env, Block *block);
    Value each_child(Env *env, Block *block);
    Value entries(Env *env); // for internal use
    int fileno(Env *env);
    Value initialize(Env *env, Value path, Optional<Value> encoding);
    Value read(Env *env);
    Value rewind(Env *env);
    Value seek(Env *env, Value position);
    nat_int_t set_pos(Env *env, Value position);
    Value tell(Env *env);

    StringObject *path(Env *env) { return m_path; }
    StringObject *inspect(Env *env);

    static bool is_empty(Env *, Value);
    static Value chdir(Env *env, Optional<Value> path, Block *block);
    static Value chroot(Env *env, Value path);
    static Value home(Env *, Optional<Value>);
    static Value mkdir(Env *env, Value path, Optional<Value> mode);
    static Value open(Env *env, Value path, Optional<Value> encoding, Block *block);
    static Value pwd(Env *env);
    static Value rmdir(Env *env, Value path); // same as .delete, .unlink

    static Value children(Env *env, Value path, Optional<Value> encoding);
    static Value entries(Env *env, Value path, Optional<Value> encoding);
    static Value each_child(Env *env, Value path, Optional<Value> encoding, Block *block);
    static Value foreach (Env *env, Value path, Optional<Value> encoding, Block * block);

private:
    DirObject()
        : Object { Object::Type::Dir, GlobalEnv::the()->Object()->const_fetch("Dir"_s).as_class() } { }

    DirObject(ClassObject *klass)
        : Object { Object::Type::Dir, klass } { }

    DirObject(const DirObject &other)
        : Object { other }
        , m_dir { other.m_dir }
        , m_path { other.m_path }
        , m_encoding { other.m_encoding } { }

    DIR *m_dir { nullptr };
    StringObject *m_path { nullptr };
    EncodingObject *m_encoding { nullptr };
};

}
