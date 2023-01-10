#include "natalie.hpp"

namespace Natalie {

EncodingObject::EncodingObject()
    : Object { Object::Type::Encoding, GlobalEnv::the()->Object()->const_fetch("Encoding"_s)->as_class() } { }

// Pretty naive implementation that doesn't support encoding options.
//
// TODO:
// * support encoding options
Value EncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const {
    StringObject temp_string = StringObject("", (EncodingObject *)this);
    ClassObject *EncodingClass = find_top_level_const(env, "Encoding"_s)->as_class();

    for (auto c : *str) {
        auto char_obj = StringObject { c, orig_encoding };
        auto source_codepoint = char_obj.ord(env)->as_integer()->to_nat_int_t();
        auto unicode_codepoint = orig_encoding->to_unicode_codepoint(source_codepoint);

        // handle error
        if (unicode_codepoint < 0) {
            StringObject *message;

            if (num() != Encoding::UTF_8) {
                // Example: "\x8F" to UTF-8 in conversion from ASCII-8BIT to UTF-8 to US-ASCII
                message = StringObject::format(
                    "\"\\x{}\" to UTF-8 in conversion from {} to UTF-8 to {}",
                    String::hex(source_codepoint, String::HexFormat::Uppercase),
                    orig_encoding->name(),
                    name());
            } else {
                message = StringObject::format(
                    "\"\\x{}\" from {} to UTF-8",
                    String::hex(source_codepoint, String::HexFormat::Uppercase),
                    orig_encoding->name());
            }

            env->raise(EncodingClass->const_find(env, "UndefinedConversionError"_s)->as_class(), message);
        }

        auto destination_codepoint = from_unicode_codepoint(unicode_codepoint);

        // handle error
        if (destination_codepoint < 0) {
            StringObject *message;

            if (orig_encoding->num() != Encoding::UTF_8)
                message = StringObject::format(
                    // Example: "U+043F to WINDOWS-1252 in conversion from Windows-1251 to UTF-8 to WINDOWS-1252",
                    "U+{} to {} in conversion from {} to UTF-8 to {}",
                    String::hex(source_codepoint, String::HexFormat::Uppercase),
                    name(),
                    orig_encoding->name(),
                    name());
            else {
                // Example: U+0439 from UTF-8 to ASCII-8BIT
                auto hex = String();
                hex.append_sprintf("%04X", source_codepoint);
                message = StringObject::format("U+{} from UTF-8 to {}", hex, name());
            }

            env->raise(EncodingClass->const_find(env, "UndefinedConversionError"_s)->as_class(), message);
        }

        auto destination_char_obj = encode_codepoint(destination_codepoint);
        temp_string.append(destination_char_obj);
    }

    str->set_str(temp_string.string().c_str(), temp_string.string().length());
    str->set_encoding(EncodingObject::get(num()));
    return str;
}

HashObject *EncodingObject::aliases(Env *env) {
    auto aliases = new HashObject();
    for (auto encoding : *list(env)) {
        auto enc = encoding->as_encoding();
        const auto &names = enc->m_names;

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
    auto string = name->as_string()->string();
    for (auto value : *list(env)) {
        auto encoding = value->as_encoding();
        for (const auto &encodingName : encoding->m_names) {
            if (encodingName.casecmp(string) == 0)
                return encoding;
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name->inspect_str(env));
}

ArrayObject *EncodingObject::list(Env *) {
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
    for (const auto &name : names)
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
    for (const auto &name : m_names)
        array->push(new StringObject { name });
    return array;
}

void EncodingObject::raise_encoding_invalid_byte_sequence_error(Env *env, const String &string, size_t index) const {
    StringObject *message = StringObject::format("invalid byte sequence at index {} in string of size {} (string not long enough)", index, string.size());
    ClassObject *InvalidByteSequenceError = fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s })->as_class();
    env->raise(InvalidByteSequenceError, message);
}

Value EncodingObject::inspect(Env *env) const {
    return StringObject::format("#<Encoding:{}>", name());
}

}
