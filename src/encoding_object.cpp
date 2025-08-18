#include "natalie/encoding_object.hpp"
#include "natalie.hpp"
#include <langinfo.h>
#include <locale.h>

namespace Natalie {

EncodingObject::EncodingObject()
    : Object { Object::Type::Encoding, GlobalEnv::the()->Object()->const_fetch("Encoding"_s).as_class() } { }

Value EncodingObject::encode(Env *env, EncodingObject *orig_encoding, StringObject *str, EncodeOptions options) const {
    if (orig_encoding->num() == Encoding::ASCII_8BIT && num() == Encoding::ASCII_8BIT)
        return str;

    ClassObject *EncodingClass = find_top_level_const(env, "Encoding"_s).as_class();
    auto temp_string = StringObject::create("", num());

    if (options.xml_option == EncodeXmlOption::Attr)
        temp_string->append_char('"');

    size_t index = 0;
    auto string = str->string();
    for (;;) {
        auto [valid, length, c] = orig_encoding->next_codepoint(string, &index);

        if (length == 0)
            break;

        switch (options.newline_option) {
        case EncodeNewlineOption::None:
            break;
        case EncodeNewlineOption::Cr:
            if (c == '\n') {
                temp_string->append_char('\r');
                continue;
            }
            break;
        case EncodeNewlineOption::Crlf:
            if (c == '\n') {
                temp_string->append("\r\n");
                continue;
            }
            break;
        case EncodeNewlineOption::Universal:
            if (c == '\r') {
                temp_string->append("\n");
                if (str->peek_char(index) == "\n")
                    index++;
                continue;
            }
            break;
        }

        switch (options.xml_option) {
        case EncodeXmlOption::None:
            break;
        case EncodeXmlOption::Attr:
            switch (c) {
            case '&':
                temp_string->append("&amp;");
                continue;
            case '<':
                temp_string->append("&lt;");
                continue;
            case '>':
                temp_string->append("&gt;");
                continue;
            case '"':
                temp_string->append("&quot;");
                continue;
            default:
                break;
            }
            break;
        case EncodeXmlOption::Text:
            switch (c) {
            case '&':
                temp_string->append("&amp;");
                continue;
            case '<':
                temp_string->append("&lt;");
                continue;
            case '>':
                temp_string->append("&gt;");
                continue;
            default:
                break;
            }
        }

        auto handle_fallback = [&](nat_int_t cpt) {
            auto ch = StringObject::create(orig_encoding->encode_codepoint(cpt));
            Value result = Value::nil();
            if (options.fallback_option.respond_to(env, "[]"_s)) {
                result = options.fallback_option.send(env, "[]"_s, { ch });
            } else if (options.fallback_option.respond_to(env, "call"_s)) {
                result = options.fallback_option.send(env, "call"_s, { ch });
            }

            if (result.is_nil()) {
                auto message = StringObject::format(
                    "U+{} from {} to {}",
                    String::hex(cpt, String::HexFormat::Uppercase),
                    orig_encoding->name(),
                    name());
                env->raise(EncodingClass->const_find(env, "UndefinedConversionError"_s)->as_class(), message);
            }

            auto result_str = result.to_str(env);
            if (result_str->encoding()->num() != num()) {
                try {
                    Value encoding = EncodingObject::get(num());
                    result_str = result_str->encode(env, encoding).to_str(env);
                } catch (ExceptionObject *e) {
                    // FIXME: I'm not sure how Ruby knows the character is "too big" for the target encoding.
                    // We don't have that kind of information yet.
                    env->raise("ArgumentError", "too big fallback string");
                }
            }
            temp_string->append(result_str);
        };

        auto source_codepoint = valid ? c : -1;

        nat_int_t unicode_codepoint = -1;
        if (source_codepoint == -1) {
            switch (options.invalid_option) {
            case EncodeInvalidOption::Raise:
                env->raise_invalid_byte_sequence_error(this);
            case EncodeInvalidOption::Replace:
                if (options.replace_option) {
                    temp_string->append(options.replace_option);
                    continue;
                }
                if (is_single_byte_encoding())
                    unicode_codepoint = '?';
                else
                    unicode_codepoint = 0xFFFD;
            }
        } else {
            unicode_codepoint = orig_encoding->to_unicode_codepoint(source_codepoint);
        }

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
            switch (options.undef_option) {
            case EncodeUndefOption::Raise:
                switch (options.xml_option) {
                case EncodeXmlOption::None:
                    break;
                case EncodeXmlOption::Attr:
                case EncodeXmlOption::Text:
                    auto entity = String::format("&#x{};", String::hex(unicode_codepoint, String::HexFormat::Uppercase));
                    temp_string->append(entity);
                    continue;
                }

                if (!options.fallback_option.is_nil()) {
                    handle_fallback(unicode_codepoint);
                    continue;
                }

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
            case EncodeUndefOption::Replace:
                if (options.replace_option) {
                    temp_string->append(options.replace_option);
                    continue;
                }
                if (is_single_byte_encoding())
                    destination_codepoint = '?';
                else
                    destination_codepoint = 0xFFFD;
            }
        }

        auto destination_char_obj = encode_codepoint(destination_codepoint);
        temp_string->append(destination_char_obj);
    }

    if (options.xml_option == EncodeXmlOption::Attr)
        temp_string->append_char('"');

    str->set_str(temp_string->string().c_str(), temp_string->string().length());
    str->set_encoding(EncodingObject::get(num()));
    return str;
}

HashObject *EncodingObject::aliases(Env *env) {
    auto aliases = HashObject::create();
    for (auto encoding : *list(env)) {
        auto enc = encoding.as_encoding();
        const auto names = enc->names(env);

        if (names->size() < 2)
            continue;

        auto original = (*names)[0]->duplicate(env);
        for (size_t i = 1; i < names->size(); ++i)
            aliases->put(env, (*names)[i]->duplicate(env), original);
    }
    return aliases;
}

EncodingObject *EncodingObject::set_default_external(Env *env, Value arg) {
    if (arg.is_encoding()) {
        s_default_external = arg.as_encoding();
    } else if (arg.is_nil()) {
        env->raise("ArgumentError", "default external cannot be nil");
    } else {
        auto name = arg.to_str(env);
        s_default_external = find(env, name).as_encoding();
    }
    s_filesystem = s_default_external;
    return default_external();
}
EncodingObject *EncodingObject::set_default_internal(Env *env, Value arg) {
    if (arg.is_encoding()) {
        s_default_internal = arg.as_encoding();
    } else if (arg.is_nil()) {
        s_default_internal = nullptr;
    } else {
        auto name = arg.to_str(env);
        s_default_internal = find(env, name).as_encoding();
    }

    return default_internal();
}

// Typically returns an EncodingObject, but can return nil.
Value EncodingObject::find(Env *env, Value name) {
    if (name.is_encoding())
        return name;
    auto string = name.to_str(env)->string().lowercase();
    if (string == "internal") {
        auto intenc = EncodingObject::default_internal();
        if (!intenc) return Value::nil();
        return intenc;
    } else if (string == "external") {
        return EncodingObject::default_external();
    } else if (string == "locale") {
        return EncodingObject::locale();
    } else if (string == "filesystem") {
        return EncodingObject::filesystem();
    }
    for (auto value : *list(env)) {
        auto encoding = value.as_encoding();
        for (const auto &encodingName : encoding->m_names) {
            if (encodingName.casecmp(string) == 0)
                return encoding;
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name.inspected(env));
}

// Lookup an EncodingObject by its string-name, or return null.
EncodingObject *EncodingObject::find_encoding_by_name(Env *env, String name) {
    auto lcase_name = name.lowercase();
    ArrayObject *list = EncodingObject::list(env);
    for (size_t i = 0; i < list->size(); i++) {
        EncodingObject *encoding = (*list)[i].as_encoding();
        ArrayObject *names = encoding->names(env);
        for (size_t n = 0; n < names->size(); n++) {
            StringObject *name_obj = (*names)[n].as_string();
            auto name = name_obj->string().lowercase();
            if (name == lcase_name) {
                return encoding;
            }
        }
    }
    return nullptr;
}

// If an EncodingObject then return it, if a StringObject,
// return the encoding matching the string.  Sets a default if the lookup fails.
EncodingObject *EncodingObject::find_encoding(Env *env, Value encoding) {
    Value enc_or_nil = EncodingObject::find(env, encoding);
    if (enc_or_nil.is_encoding())
        return enc_or_nil.as_encoding();

    auto enc = EncodingObject::find_encoding_by_name(env, String("BINARY"));
    if (!enc)
        env->raise("ArgumentError", "unknown encoding name - {}", encoding.inspected(env));

    return enc;
}

ArrayObject *EncodingObject::list(Env *) {
    auto ary = ArrayObject::create(EncodingCount);
    for (auto encoding : s_encoding_list)
        ary->push(encoding);
    // dbg("size {} enccnt {}", ary->size(), EncodingCount);
    assert(ary->size() == EncodingCount);
    return ary;
}

ArrayObject *EncodingObject::name_list(Env *env) {
    size_t size = 0;
    for (const auto &encoding : s_encoding_list)
        size += encoding->m_names.size();
    auto ary = ArrayObject::create(size);
    for (const auto encoding : s_encoding_list)
        ary->concat(*encoding->names(env));
    return ary;
}

EncodingObject::EncodingObject(Encoding num, std::initializer_list<const String> names)
    : EncodingObject {} {
    const auto index = static_cast<size_t>(num) - 1;
    assert(s_encoding_list.at(index) == nullptr);
    m_num = num;
    for (const auto &name : names)
        m_names.push(name);
    s_encoding_list[index] = this;
}

Value EncodingObject::name(Env *env) {
    return StringObject::create(m_names[0]);
}

const StringObject *EncodingObject::name() const {
    return StringObject::create(m_names[0]);
}

ArrayObject *EncodingObject::names(Env *env) const {
    auto array = ArrayObject::create(m_names.size());
    for (const auto &name : m_names)
        array->push(StringObject::create(name));
    if (this == s_locale) array->push(StringObject::create("locale"));
    if (this == s_default_external) array->push(StringObject::create("external"));
    if (this == s_filesystem) array->push(StringObject::create("filesystem"));
    if (this == s_default_internal) array->push(StringObject::create("internal"));
    return array;
}

void EncodingObject::raise_encoding_invalid_byte_sequence_error(Env *env, const String &string, size_t index) const {
    StringObject *message = StringObject::format("invalid byte sequence at index {} in string of size {} (string not long enough)", index, string.size());
    ClassObject *InvalidByteSequenceError = fetch_nested_const({ "Encoding"_s, "InvalidByteSequenceError"_s }).as_class();
    env->raise(InvalidByteSequenceError, message);
}

void EncodingObject::raise_compatibility_error(Env *env, const EncodingObject *other_encoding) const {
    auto exception_class = fetch_nested_const({ "Encoding"_s, "CompatibilityError"_s }).as_class();
    env->raise(exception_class, "incompatible character encodings: {} and {}", name()->string(), other_encoding->name()->string());
}

Value EncodingObject::inspect(Env *env) const {
    if (is_dummy())
        return StringObject::format("#<Encoding:{} (dummy)>", name());
    if (m_num == Encoding::ASCII_8BIT)
        return StringObject::format("#<Encoding:BINARY ({})>", name());
    return StringObject::format("#<Encoding:{}>", name());
}

// Default implementation is pretty dumb; we'll need to override this for
// every multi-byte encoding, I think.
std::tuple<bool, int, nat_int_t> EncodingObject::next_codepoint(const String &string, size_t *index) const {
    auto [valid, view] = next_char(string, index);
    auto codepoint = view.is_empty() ? 0 : decode_codepoint(view);
    return { valid, view.length(), codepoint };
}

Value EncodingObject::locale_charmap() {
    auto codeset = ::nl_langinfo(CODESET);
    assert(codeset);
    return StringObject::create(codeset, Encoding::US_ASCII);
}

void EncodingObject::initialize_defaults(Env *env) {
    ::setlocale(LC_CTYPE, "");
    auto codestr = EncodingObject::locale_charmap();
    s_locale = EncodingObject::find(env, codestr).as_encoding();
    // NATFIXME: find a way to get -E option (which forces external encoding)
    // to factor into this default external encoding
    s_default_external = s_locale;
    s_filesystem = s_default_external;
}

uint8_t EncodingObject::codepoint_to_lowercase(nat_int_t codepoint, nat_int_t result[], uint8_t flags) {
    if (flags & CaseMapAscii) {
        if (codepoint >= 'A' && codepoint <= 'Z')
            result[0] = codepoint + 32;
        else
            result[0] = codepoint;
        return 1;
    }

    auto block = codepoint >> 8;
    auto index = lcase_index[block] + (codepoint & 0xff);
    auto delta = lcase_map[index];
    if (delta != 0) {
        result[0] = codepoint + delta;
        return 1;
    }

    if (special_casing_map[0].code == 0)
        init_special_casing_map();
    auto entry = find_special_casing_map_entry(codepoint);
    if (entry.code != 0) {
        int i = 0;
        for (i = 0; i < SPECIAL_CASE_LOWER_MAX_SIZE; i++) {
            if (entry.lower[i] == 0) break;
            result[i] = entry.lower[i];
        }
        return i;
    }

    result[0] = codepoint;
    return 1;
}

uint8_t EncodingObject::codepoint_to_uppercase(nat_int_t codepoint, nat_int_t result[], uint8_t flags) {
    if (flags & CaseMapAscii) {
        if (codepoint >= 'a' && codepoint <= 'z')
            result[0] = codepoint - 32;
        else
            result[0] = codepoint;
        return 1;
    }

    if (flags & CaseMapTurkicAzeri && codepoint == 0x69) {
        result[0] = 0x130;
        return 1;
    }

    auto block = codepoint >> 8;
    auto index = ucase_index[block] + (codepoint & 0xff);
    auto delta = ucase_map[index];
    if (delta != 0) {
        result[0] = codepoint + delta;
        return 1;
    }

    if (special_casing_map[0].code == 0)
        init_special_casing_map();
    auto entry = find_special_casing_map_entry(codepoint);
    if (entry.code != 0) {
        int i = 0;
        for (i = 0; i < SPECIAL_CASE_UPPER_MAX_SIZE; i++) {
            if (entry.upper[i] == 0) break;
            result[i] = entry.upper[i];
        }
        return i;
    }

    result[0] = codepoint;
    return 1;
}

uint8_t EncodingObject::codepoint_to_titlecase(nat_int_t codepoint, nat_int_t result[], uint8_t flags) {
    if (flags & CaseMapAscii)
        return codepoint_to_uppercase(codepoint, result, CaseMapAscii);

    if (flags & CaseMapTurkicAzeri && codepoint == 0x69) {
        result[0] = 0x130;
        return 1;
    }

    auto block = codepoint >> 8;
    auto index = tcase_index[block] + (codepoint & 0xff);
    auto delta = tcase_map[index];
    if (delta != 0) {
        result[0] = codepoint + delta;
        return 1;
    }

    if (special_casing_map[0].code == 0)
        init_special_casing_map();
    auto entry = find_special_casing_map_entry(codepoint);
    if (entry.code != 0) {
        int i = 0;
        for (i = 0; i < SPECIAL_CASE_TITLE_MAX_SIZE; i++) {
            if (entry.title[i] == 0) break;
            result[i] = entry.title[i];
        }
        return i;
    }

    result[0] = codepoint;
    return 1;
}

SpecialCasingEntry EncodingObject::find_special_casing_map_entry(nat_int_t codepoint) {
    int low = 0;
    int high = special_casing_map_size - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (special_casing_map[mid].code == codepoint)
            return special_casing_map[mid];
        if (special_casing_map[mid].code < codepoint)
            low = mid + 1;
        else
            high = mid - 1;
    }

    return {};
}

bool EncodingObject::is_printable_char(const nat_int_t c) const {
    return (c >= 32 && c < 127) || c >= 256;
}

bool EncodingObject::is_compatible_with(EncodingObject *other_encoding) const {
    if (other_encoding == this) return true;

    return is_ascii_compatible() && other_encoding->is_ascii_compatible();
}

}
