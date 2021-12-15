#include "natalie.hpp"

namespace Natalie {

EncodingObject::EncodingObject()
    : Object { Object::Type::Encoding, GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Encoding"))->as_class() } { }

ArrayObject *EncodingObject::list(Env *) {
    ArrayObject *ary = new ArrayObject {};
    Value Encoding = GlobalEnv::the()->Object()->const_fetch(SymbolObject::intern("Encoding"));
    ary->push(Encoding->const_fetch(SymbolObject::intern("ASCII_8BIT")));
    ary->push(Encoding->const_fetch(SymbolObject::intern("UTF_8")));
    return ary;
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
    auto array = new ArrayObject {};
    for (StringObject *name : m_names) {
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
