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

EncodingObject *EncodingObject::set_default_internal(Env *env, Value arg) {
    if (arg->is_encoding()) {
        s_default_internal = arg->as_encoding();
    } else if (arg->is_nil()) {
        s_default_internal = nullptr;
    } else {
        auto name = arg->to_str(env);
        s_default_internal = find(env, name);
    }

    return default_internal();
}

EncodingObject *EncodingObject::find(Env *env, Value name) {
    for (auto value : *list(env)) {
        auto encoding = value->as_encoding();
        for (auto encodingName : encoding->m_names) {
            auto string = name->as_string();
            if (strcasecmp(string->c_str(), encodingName->c_str()) == 0)
                return encoding;
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name->inspect_str(env));
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
    assert(s_encoding_list.get(num) == nullptr);
    m_num = num;
    for (const char *name : names) {
        m_names.push(new StringObject { name });
    }
    s_encoding_list.put(num, this);
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
    return StringObject::format("#<Encoding:{}>", name());
}

void EncodingObject::visit_children(Visitor &visitor) {
    Object::visit_children(visitor);
    for (auto name : m_names) {
        visitor.visit(name);
    }
}

}
