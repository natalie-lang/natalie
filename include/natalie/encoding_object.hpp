#pragma once

#include <assert.h>
#include <initializer_list>

#include "natalie/array_object.hpp"
#include "natalie/class_object.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/object.hpp"

namespace Natalie {

using namespace TM;

enum class Encoding {
    ASCII_8BIT = 1,
    UTF_8 = 2,
};

class EncodingObject : public Object {
public:
    EncodingObject();

    EncodingObject(ClassObject *klass)
        : Object { Object::Type::Encoding, klass } { }

    EncodingObject(Encoding, std::initializer_list<const char *>);

    Encoding num() { return m_num; }

    const StringObject *name() { return m_names[0]; }
    Value name(Env *);

    ArrayObject *names(Env *);

    Value inspect(Env *);

    static HashObject *aliases(Env *);
    static ArrayObject *list(Env *env);

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<EncodingObject %p>", this);
    }

private:
    Vector<StringObject *> m_names {};
    Encoding m_num;
};

EncodingObject *encoding(Env *env, Encoding num, ArrayObject *names);

}
