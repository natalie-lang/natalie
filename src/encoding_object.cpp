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

        auto original = new StringObject { names[0] };
        for (size_t i = 1; i < names.size(); ++i)
            aliases->put(env, new StringObject { names[i] }, original);
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
    auto string = name->as_string()->to_low_level_string();
    for (auto value : *list(env)) {
        auto encoding = value->as_encoding();
        for (auto encodingName : encoding->m_names) {
            if (encodingName.casecmp(string) == 0)
                return encoding;
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name->inspect_str(env));
}

ArrayObject *EncodingObject::list(Env *) {
    Value Encoding = GlobalEnv::the()->Object()->const_fetch("Encoding"_s);
    auto ary = new ArrayObject {};
    for (auto pair : s_encoding_list)
        ary->push(pair.second);
    assert(ary->size() == EncodingCount);
    return ary;
}

EncodingObject::EncodingObject(Encoding num, std::initializer_list<const String> names)
    : EncodingObject {} {
    assert(s_encoding_list.get(num) == nullptr);
    m_num = num;
    for (auto name : names)
        m_names.push(name);
    s_encoding_list.put(num, this);
}

Value EncodingObject::name(Env *env) {
    return new StringObject { m_names[0] };
}

const StringObject *EncodingObject::name() const {
    return new StringObject { m_names[0] };
}

ArrayObject *EncodingObject::names(Env *env) {
    auto array = new ArrayObject { m_names.size() };
    for (auto name : m_names)
        array->push(new StringObject { name });
    return array;
}

void EncodingObject::raise_encoding_invalid_byte_sequence_error(const String &string, size_t index) const {
    StringObject *message = StringObject::format("invalid byte sequence at index {} in string of size {} (string not long enough)", index, string.size());
    ClassObject *InvalidByteSequenceError = fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s })->as_class();
    throw new ExceptionObject { InvalidByteSequenceError, message };
}

Value EncodingObject::inspect(Env *env) const {
    return StringObject::format("#<Encoding:{}>", name());
}

}
