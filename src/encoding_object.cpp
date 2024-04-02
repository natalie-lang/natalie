#include "natalie.hpp"
#include <langinfo.h>
#include <locale.h>

namespace Natalie {

EncodingObject::EncodingObject()
    : Object { Object::Type::Encoding, GlobalEnv::the()->Object()->const_fetch("Encoding"_s)->as_class() } { }

// Pretty naive implementation that doesn't support encoding options.
//
// TODO:
// * support encoding options
Value EncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str) const {
    if (num() == orig_encoding->num())
        return str;

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
        const auto names = enc->names(env);

        if (names->size() < 2)
            continue;

        auto original = (*names)[0]->dup(env);
        for (size_t i = 1; i < names->size(); ++i)
            aliases->put(env, (*names)[i]->dup(env), original);
    }
    return aliases;
}

EncodingObject *EncodingObject::set_default_external(Env *env, Value arg) {
    if (arg->is_encoding()) {
        s_default_external = arg->as_encoding();
    } else if (arg->is_nil()) {
        env->raise("ArgumentError", "default external cannot be nil");
    } else {
        auto name = arg->to_str(env);
        s_default_external = find(env, name)->as_encoding();
    }
    s_filesystem = s_default_external;
    return default_external();
}
EncodingObject *EncodingObject::set_default_internal(Env *env, Value arg) {
    if (arg->is_encoding()) {
        s_default_internal = arg->as_encoding();
    } else if (arg->is_nil()) {
        s_default_internal = nullptr;
    } else {
        auto name = arg->to_str(env);
        s_default_internal = find(env, name)->as_encoding();
    }

    return default_internal();
}

// Typically returns an EncodingObject, but can return nil.
Value EncodingObject::find(Env *env, Value name) {
    if (name->is_encoding())
        return name;
    auto string = name->to_str(env)->string().lowercase();
    if (string == "internal") {
        auto intenc = EncodingObject::default_internal();
        if (!intenc) return NilObject::the();
        return intenc;
    } else if (string == "external") {
        return EncodingObject::default_external();
    } else if (string == "locale") {
        return EncodingObject::locale();
    } else if (string == "filesystem") {
        return EncodingObject::filesystem();
    }
    for (auto value : *list(env)) {
        auto encoding = value->as_encoding();
        for (const auto &encodingName : encoding->m_names) {
            if (encodingName.casecmp(string) == 0)
                return encoding;
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name->inspect_str(env));
}

// Lookup an EncodingObject by its string-name, or raise if unsuccessful.
EncodingObject *EncodingObject::find_encoding_by_name(Env *env, String name) {
    auto lcase_name = name.lowercase();
    ArrayObject *list = EncodingObject::list(env);
    for (size_t i = 0; i < list->size(); i++) {
        EncodingObject *encoding = (*list)[i]->as_encoding();
        ArrayObject *names = encoding->names(env);
        for (size_t n = 0; n < names->size(); n++) {
            StringObject *name_obj = (*names)[n]->as_string();
            auto name = name_obj->string().lowercase();
            if (name == lcase_name) {
                return encoding;
            }
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name);
}

// If an EncodingObject then return it, if a StringObject,
// return the encoding matching the string.  Sets a default if the lookup fails.
EncodingObject *EncodingObject::find_encoding(Env *env, Value encoding) {
    Value enc_or_nil = EncodingObject::find(env, encoding);
    if (enc_or_nil->is_encoding())
        return enc_or_nil->as_encoding();
    return EncodingObject::find_encoding_by_name(env, String("BINARY"));
}

ArrayObject *EncodingObject::list(Env *) {
    auto ary = new ArrayObject { EncodingCount };
    for (auto pair : s_encoding_list)
        ary->push(pair.second);
    // dbg("size {} enccnt {}", ary->size(), EncodingCount);
    assert(ary->size() == EncodingCount);
    return ary;
}

ArrayObject *EncodingObject::name_list(Env *env) {
    auto ary = new ArrayObject {};
    for (auto pair : s_encoding_list)
        ary->concat(*pair.second->names(env));
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

ArrayObject *EncodingObject::names(Env *env) const {
    auto array = new ArrayObject { m_names.size() };
    for (const auto &name : m_names)
        array->push(new StringObject { name });
    if (this == s_locale) array->push(new StringObject { "locale" });
    if (this == s_default_external) array->push(new StringObject { "external" });
    if (this == s_filesystem) array->push(new StringObject { "filesystem" });
    if (this == s_default_internal) array->push(new StringObject { "internal" });
    return array;
}

void EncodingObject::raise_encoding_invalid_byte_sequence_error(Env *env, const String &string, size_t index) const {
    StringObject *message = StringObject::format("invalid byte sequence at index {} in string of size {} (string not long enough)", index, string.size());
    ClassObject *InvalidByteSequenceError = fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s })->as_class();
    env->raise(InvalidByteSequenceError, message);
}

Value EncodingObject::inspect(Env *env) const {
    if (is_dummy())
        return StringObject::format("#<Encoding:{} (dummy)>", name());
    return StringObject::format("#<Encoding:{}>", name());
}

Value EncodingObject::locale_charmap() {
    auto codeset = ::nl_langinfo(CODESET);
    assert(codeset);
    return new StringObject { codeset, Encoding::US_ASCII };
}

void EncodingObject::initialize_defaults(Env *env) {
    ::setlocale(LC_CTYPE, "");
    auto codestr = EncodingObject::locale_charmap();
    s_locale = EncodingObject::find(env, codestr)->as_encoding();
    // NATFIXME: find a way to get -E option (which forces external encoding)
    // to factor into this default external encoding
    s_default_external = s_locale;
    s_filesystem = s_default_external;
}

nat_int_t EncodingObject::codepoint_to_lowercase(nat_int_t codepoint) {
    auto block = codepoint >> 8;
    auto index = lcase_index[block] + (codepoint & 0xff);
    auto delta = lcase_map[index];
    if (delta == 0)
        return 0;
    return codepoint + delta;
}

nat_int_t EncodingObject::codepoint_to_uppercase(nat_int_t codepoint) {
    auto block = codepoint >> 8;
    auto index = ucase_index[block] + (codepoint & 0xff);
    auto delta = ucase_map[index];
    if (delta == 0)
        return 0;
    return codepoint + delta;
}

nat_int_t EncodingObject::codepoint_to_titlecase(nat_int_t codepoint) {
    auto block = codepoint >> 8;
    auto index = tcase_index[block] + (codepoint & 0xff);
    auto delta = tcase_map[index];
    if (delta == 0)
        return 0;
    return codepoint + delta;
}

bool EncodingObject::is_printable_char(const nat_int_t c) const {
    return (c >= 32 && c < 127) || c >= 256;
}

bool EncodingObject::is_compatible_with(EncodingObject *other_encoding) const {
    if (other_encoding == this) return true;

    return is_ascii_compatible() && other_encoding->is_ascii_compatible();
}

}
