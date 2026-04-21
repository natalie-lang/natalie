#include "natalie/encoding_object.hpp"
#include "natalie.hpp"
#include <langinfo.h>
#include <locale.h>

namespace Natalie {

EncodingObject::EncodingObject()
    : Object { Object::Type::Encoding, GlobalEnv::the()->Object()->const_fetch("Encoding"_s).as_class() } { }

// Returns the encoding of an object, or nullptr if it doesn't have one
static EncodingObject *encoding_of(Env *env, Value obj) {
    if (obj.is_string())
        return obj.as_string()->encoding();
    if (obj.is_encoding())
        return obj.as_encoding();
    if (obj.is_symbol())
        return obj.as_symbol()->encoding(env);
    if (obj.is_regexp())
        return obj.as_regexp()->encoding();
    return nullptr;
}

static bool is_ascii_only(Env *env, Value obj) {
    if (obj.is_string())
        return obj.as_string()->is_ascii_only();
    if (obj.is_symbol()) {
        auto enc = obj.as_symbol()->encoding(env);
        return enc->num() == Encoding::US_ASCII;
    }
    return false;
}

EncodingObject *EncodingObject::compatible(EncodingObject *enc1, bool ascii1, EncodingObject *enc2, bool ascii2) {
    if (enc1 == enc2)
        return enc1;

    if (!enc1->is_ascii_compatible() || !enc2->is_ascii_compatible())
        return nullptr;

    if (ascii1)
        return ascii2 ? enc1 : enc2;
    if (ascii2)
        return enc1;

    return nullptr;
}

EncodingObject *EncodingObject::compatible(const StringObject *s1, const StringObject *s2) {
    if (s2->is_empty())
        return s1->encoding();

    if (s1->is_empty()) {
        auto enc1 = s1->encoding();
        return (enc1->is_ascii_compatible() && s2->is_ascii_only()) ? enc1 : s2->encoding();
    }

    return compatible(s1->encoding(), s1->is_ascii_only(), s2->encoding(), s2->is_ascii_only());
}

Value EncodingObject::compatible(Env *env, Value obj1, Value obj2) {
    auto enc1 = encoding_of(env, obj1);
    auto enc2 = encoding_of(env, obj2);

    if (!enc1 || !enc2)
        return Value::nil();

    if (enc1 == enc2)
        return enc1;

    const bool isstr1 = obj1.is_string();
    const bool isstr2 = obj2.is_string();

    if (isstr1 && isstr2) {
        auto enc = compatible(obj1.as_string(), obj2.as_string());
        return enc ? Value(enc) : Value::nil();
    }

    if (isstr2 && obj2.as_string()->is_empty())
        return enc1;

    if (!enc1->is_ascii_compatible() || !enc2->is_ascii_compatible())
        return Value::nil();

    if (!isstr2 && enc2->num() == Encoding::US_ASCII)
        return enc1;
    if (!isstr1 && enc1->num() == Encoding::US_ASCII)
        return enc2;

    if (!isstr1 && !isstr2) {
        if (is_ascii_only(env, obj1) && !is_ascii_only(env, obj2)) return enc2;
        if (!is_ascii_only(env, obj1) && is_ascii_only(env, obj2)) return enc1;
        return Value::nil();
    }

    // A 7-bit string takes the position-2 encoding.
    auto str_obj = isstr1 ? obj1.as_string() : obj2.as_string();
    if (str_obj->is_ascii_only())
        return enc2;

    return Value::nil();
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
    env->raise("ArgumentError", "unknown encoding name - {}", name.to_s(env)->string());
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
        env->raise("ArgumentError", "unknown encoding name - {}", encoding.to_s(env)->string());

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

    if (flags & CaseMapTurkicAzeri) {
        // 'I' -> 'ı'
        if (codepoint == 'I') {
            result[0] = 0x131;
            return 1;
        }
        // 'İ' -> 'i'
        if (codepoint == 0x130) {
            result[0] = 'i';
            return 1;
        }
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

    if (flags & CaseMapTurkicAzeri) {
        // 'i' -> 'İ'
        if (codepoint == 'i') {
            result[0] = 0x130;
            return 1;
        }
        // 'ı' -> 'I'
        if (codepoint == 0x131) {
            result[0] = 'I';
            return 1;
        }
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

static uint8_t copy_special_case_entry(const uint32_t *entry_chars, int max, nat_int_t result[]) {
    int i = 0;
    for (i = 0; i < max; i++) {
        if (entry_chars[i] == 0) break;
        result[i] = entry_chars[i];
    }
    return i;
}

uint8_t EncodingObject::codepoint_to_swapcase(nat_int_t codepoint, nat_int_t result[], uint8_t flags) {
    if (flags & CaseMapAscii) {
        if (codepoint >= 'A' && codepoint <= 'Z')
            result[0] = codepoint + 32;
        else if (codepoint >= 'a' && codepoint <= 'z')
            result[0] = codepoint - 32;
        else
            result[0] = codepoint;
        return 1;
    }

    if (flags & CaseMapTurkicAzeri) {
        // 'i' -> 'İ'
        if (codepoint == 'i') {
            result[0] = 0x130;
            return 1;
        }
        // 'İ' -> 'i'
        if (codepoint == 0x130) {
            result[0] = 'i';
            return 1;
        }
        // 'I' -> 'ı'
        if (codepoint == 'I') {
            result[0] = 0x131;
            return 1;
        }
        // 'ı' -> 'I'
        if (codepoint == 0x131) {
            result[0] = 'I';
            return 1;
        }
    }

    auto block = codepoint >> 8;
    auto lcase_delta = lcase_map[lcase_index[block] + (codepoint & 0xff)];
    if (lcase_delta != 0) {
        result[0] = codepoint + lcase_delta;
        return 1;
    }
    auto ucase_delta = ucase_map[ucase_index[block] + (codepoint & 0xff)];
    if (ucase_delta != 0) {
        result[0] = codepoint + ucase_delta;
        return 1;
    }

    if (special_casing_map[0].code == 0)
        init_special_casing_map();
    auto entry = find_special_casing_map_entry(codepoint);
    if (entry.code != 0) {
        // Pick whichever side has a non-trivial mapping (the other side maps to
        // itself).
        if (entry.upper[0] != 0 && entry.upper[0] != (uint32_t)codepoint)
            return copy_special_case_entry(entry.upper, SPECIAL_CASE_UPPER_MAX_SIZE, result);
        if (entry.lower[0] != 0 && entry.lower[0] != (uint32_t)codepoint)
            return copy_special_case_entry(entry.lower, SPECIAL_CASE_LOWER_MAX_SIZE, result);
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
