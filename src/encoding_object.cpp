#include "natalie.hpp"

namespace Natalie {

EncodingObject::EncodingObject()
    : Object { Object::Type::Encoding, GlobalEnv::the()->Object()->const_fetch("Encoding"_s)->as_class() } { }

HashObject *EncodingObject::aliases(Env *env) {
    auto aliases = new HashObject();
    for (auto encoding : *list(env)) {
        auto enc = encoding->as_encoding();
        auto names = enc->m_names;

        if (names.size() < 2)
            continue;

        auto original = names[0]->dup(env);
        for (size_t i = 1; i < names.size(); ++i)
            aliases->put(env, names[i]->dup(env), original);
    }
    return aliases;
}

ArrayObject *EncodingObject::list(Env *) {
    Value Encoding = GlobalEnv::the()->Object()->const_fetch("Encoding"_s);
    return new ArrayObject {
        Encoding->const_fetch("ASCII_8BIT"_s),
        Encoding->const_fetch("UTF_8"_s)
    };
}

EncodingObject::EncodingObject(Encoding num, std::initializer_list<const char *> names)
    : EncodingObject {} {
    m_num = num;
    for (const char *name : names) {
        m_names.push(new StringObject { name });
    }
}

Value EncodingObject::name(Env *env) {
    return m_names[0]->dup(env);
}

ArrayObject *EncodingObject::names(Env *env) {
    auto array = new ArrayObject { m_names.size() };
    for (auto name : m_names) {
        array->push(name);
    }
    return array;
}

Value EncodingObject::inspect(Env *env) {
    return StringObject::format(env, "#<Encoding:{}>", name());
}

void EncodingObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    for (auto name : m_names) {
        visitor.visit(name);
    }
}

}
