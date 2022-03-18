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

    bool in_encoding_codepoint_range(nat_int_t codepoint) {
        switch (m_num) {
        case Encoding::ASCII_8BIT:
            return codepoint >= 0 && codepoint < 256;
        case Encoding::UTF_8:
            return codepoint >= 0 && codepoint < 1114112;
        }
        NAT_UNREACHABLE();
    }

    // NATFIXME: Check if a codepoint is invalid
    bool invalid_codepoint(nat_int_t codepoint) {
        return false;
    }

    static HashObject *aliases(Env *);
    static EncodingObject *find(Env *, Value);
    static ArrayObject *list(Env *env);
    static const TM::Hashmap<Encoding, EncodingObject *> &encodings() { return EncodingObject::s_encoding_list; }

    virtual void visit_children(Visitor &) override final;

    virtual void gc_inspect(char *buf, size_t len) const override {
        snprintf(buf, len, "<EncodingObject %p>", this);
    }

private:
    Vector<StringObject *> m_names {};
    Encoding m_num;

    static inline TM::Hashmap<Encoding, EncodingObject *> s_encoding_list {};
};

EncodingObject *encoding(Env *env, Encoding num, ArrayObject *names);

}
