#include "natalie/string_object.hpp"
#include "ctype.h"
#include "natalie.hpp"
#include "natalie/crypt.h"
#include "natalie/integer_methods.hpp"
#include "natalie/string_unpacker.hpp"
#include "string.h"

#include <unistd.h>

namespace Natalie {

constexpr bool is_strippable_whitespace(char c) {
    return c == '\0'
        || c == '\t'
        || c == '\n'
        || c == '\v'
        || c == '\f'
        || c == '\r'
        || c == ' ';
};

static auto character_class_handler(Env *env, Args &&args) {
    args.ensure_argc_at_least(env, 1);

    auto basic_characters = Hashmap<String>();
    auto negated_characters = Hashmap<String>();

    // For each argument
    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = args[i];

        // Try convert to string
        auto selectors = arg.to_str(env);
        auto new_selectors = Hashmap<String>();
        StringView last_character = {};
        bool negated = false;

        // Split into selectors
        size_t index = 0;
        auto result = selectors->next_char_result(&index);
        if (result.second == "^") {
            if (index != selectors->bytesize())
                negated = true;
            else
                new_selectors.set("^");
            result = selectors->next_char_result(&index);
        }
        while (!result.second.is_empty()) {
            if (!result.first)
                env->raise_invalid_byte_sequence_error(selectors->encoding());

            auto value = result.second;
            if (value == "-" && last_character != StringView() && last_character != "\\") {
                result = selectors->next_char_result(&index);
                if (result.second.is_empty()) {
                    new_selectors.set(value);
                    break;
                }

                auto next_value = result.second;
                if (!result.first)
                    env->raise("ArgumentError", "invalid range \"{}-{}\" in string transliteration", last_character, next_value);

                if (last_character.to_string().cmp(next_value.to_string()) == 1)
                    env->raise("ArgumentError", "invalid range \"{}-{}\" in string transliteration", last_character, next_value);

                auto range = RangeObject::create(env, new StringObject { last_character }, new StringObject { next_value }, false);
                auto all_chars = range->to_a(env).as_array();
                for (auto character : *all_chars) {
                    auto character_string = character.as_string()->string();
                    new_selectors.set(character_string);
                }
                last_character = StringView();
            } else {
                last_character = value;
                new_selectors.set(value);
            }
            result = selectors->next_char_result(&index);
        }

        // intersect with current selectors
        if (negated) {
            for (auto pair : new_selectors) {
                negated_characters.set(pair.first);
            }
        } else {
            auto new_basic_characters = Hashmap<String>();
            for (auto pair : new_selectors) {
                if (basic_characters.is_empty() || basic_characters.get(pair.first) != nullptr) {
                    new_basic_characters.set(pair.first);
                }
            }
            basic_characters = new_basic_characters;
        }
    }

    return [basic_characters, negated_characters](const StringView character) -> bool {
        if (basic_characters.is_empty() && negated_characters.is_empty())
            return false;
        return ((negated_characters.is_empty() || negated_characters.get(character.to_string()) == nullptr)
            && (basic_characters.is_empty() || basic_characters.get(character.to_string()) != nullptr));
    };
}

std::pair<bool, StringView> StringObject::prev_char_result(size_t *index) const {
    return m_encoding->prev_char(m_string, index);
}

StringView StringObject::prev_char(size_t *index) const {
    return prev_char_result(index).second;
}

StringView StringObject::prev_char(Env *env, size_t *index) const {
    auto pair = prev_char_result(index);
    if (!pair.first)
        m_encoding->raise_encoding_invalid_byte_sequence_error(env, m_string, *index);
    return pair.second;
}

std::pair<bool, StringView> StringObject::peek_char_result(size_t index) const {
    return m_encoding->next_char(m_string, &index);
}

std::pair<bool, StringView> StringObject::next_char_result(size_t *index) const {
    return m_encoding->next_char(m_string, index);
}

StringView StringObject::peek_char(size_t index) const {
    return next_char_result(&index).second;
}

StringView StringObject::next_char(size_t *index) const {
    return next_char_result(index).second;
}

StringView StringObject::next_char(Env *env, size_t *index) const {
    auto pair = next_char_result(index);
    if (!pair.first)
        m_encoding->raise_encoding_invalid_byte_sequence_error(env, m_string, *index);
    return pair.second;
}

Value StringObject::each_char(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, StringObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_char"_s }, size_block);
    }

    for (auto c : *this) {
        Value args[] = { new StringObject { c, m_encoding } };
        block->run(env, Args(1, args), nullptr);
    }
    return this;
}

Value StringObject::chars(Env *env, Block *block) {
    if (block) {
        for (auto c : *this) {
            auto str = new StringObject { c, m_encoding };
            block->run(env, Args({ str }), nullptr);
        }
        return this;
    }
    ArrayObject *ary = new ArrayObject {};
    for (auto c : *this)
        ary->push(new StringObject { c, m_encoding });
    return ary;
}

Value StringObject::each_codepoint(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, StringObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_codepoint"_s }, size_block);
    }

    for (auto c : *this) {
        auto char_obj = StringObject { c, m_encoding };

        if (!char_obj.valid_encoding())
            env->raise_invalid_byte_sequence_error(m_encoding.ptr());

        Value args[] = { char_obj.ord(env) };
        block->run(env, Args(1, args), nullptr);
    }

    return this;
}

Value StringObject::codepoints(Env *env, Block *block) {
    size_t index = 0;

    if (block) {
        for (;;) {
            auto [valid, length, codepoint] = m_encoding->next_codepoint(m_string, &index);
            if (!valid)
                env->raise_invalid_byte_sequence_error(m_encoding.ptr());
            if (length == 0)
                break;
            Value args[] = { Value::integer(codepoint) };
            block->run(env, Args(1, args), nullptr);
        }
        return this;
    }

    if (!this->valid_encoding())
        env->raise_invalid_byte_sequence_error(m_encoding.ptr());

    ArrayObject *ary = new ArrayObject {};
    for (;;) {
        auto [valid, length, codepoint] = m_encoding->next_codepoint(m_string, &index);
        if (!valid)
            env->raise_invalid_byte_sequence_error(m_encoding.ptr());
        if (length == 0)
            break;
        ary->push(Value::integer(codepoint));
    }
    return ary;
}

Value StringObject::each_grapheme_cluster(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, StringObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_grapheme_cluster"_s }, size_block);
    }

    size_t index = 0;
    for (;;) {
        auto view = m_encoding->next_grapheme_cluster(m_string, &index);
        if (view.is_empty())
            break;
        Value args[] = { new StringObject { view, m_encoding } };
        block->run(env, Args(1, args), nullptr);
    }
    return this;
}

Value StringObject::grapheme_clusters(Env *env, Block *block) {
    if (block)
        return each_grapheme_cluster(env, block);

    auto ary = new ArrayObject {};
    size_t index = 0;
    for (;;) {
        auto view = m_encoding->next_grapheme_cluster(m_string, &index);
        if (view.is_empty())
            break;
        ary->push(new StringObject { view, m_encoding });
    }
    return ary;
}

String create_padding(String &padding, size_t length) {
    size_t quotient = ::floor(length / padding.size());
    size_t remainder = length % padding.size();
    auto buffer = String("");
    for (size_t i = 0; i < quotient; ++i)
        buffer.append(padding);
    for (size_t j = 0; j < remainder; ++j)
        buffer.append_char(padding[j]);
    return buffer;
}

Value StringObject::center(Env *env, Value length, Value padstr) {
    nat_int_t length_i = length.to_int(env).to_nat_int_t();

    String pad;

    if (!padstr) {
        pad = String { " " };
    } else if (padstr.is_string()) {
        pad = padstr.as_string()->string();
    } else {
        pad = padstr.to_str(env)->string();
    }

    if (pad.is_empty())
        env->raise("ArgumentError", "zero width padding");

    if (length_i <= (nat_int_t)m_string.size())
        return this;

    double split = (length_i - m_string.size()) / 2.0;
    auto left_split = ::floor(split);
    auto right_split = ::ceil(split);

    auto result = String { m_string };
    result.prepend(create_padding(pad, left_split));
    result.append(create_padding(pad, right_split));

    return new StringObject { result, m_encoding };
}

Value StringObject::chomp(Env *env, Value record_separator) const {
    auto new_str = new StringObject { m_string, m_encoding };
    new_str->chomp_in_place(env, record_separator);
    return new_str;
}

Value StringObject::chomp_in_place(Env *env, Value record_separator) {
    assert_not_frozen(env);

    // When passed nil, return nil
    if (!record_separator.is_null() && record_separator.is_nil()) {
        return Value::nil();
    }

    // When passed a non nil object, call to_str();
    if (!record_separator.is_null() && !record_separator.is_string()) {
        record_separator = record_separator.to_str(env);
    }

    if (is_empty()) { // if this is an empty string, return nil
        return Value::nil();
    }

    size_t end_idx = m_string.length();
    String global_record_separator = env->global_get("$/"_s).as_string()->string();
    // When using default record separator, also remove trailing \r
    if (record_separator.is_null() && global_record_separator == "\n") {
        size_t char_pos = end_idx;
        auto removed_char = prev_char(&char_pos);
        auto last_codept = m_encoding->decode_codepoint(removed_char);
        if (last_codept == 0x0D) { // CR
            end_idx = char_pos;
        } else if (last_codept == 0x0A) { // LF
            end_idx = char_pos;
            if (char_pos > 0) {
                removed_char = prev_char(&char_pos);
                last_codept = m_encoding->decode_codepoint(removed_char);
                if (last_codept == 0x0D) // CR
                    end_idx = char_pos;
            }
        }

        if (end_idx == m_string.length()) {
            return Value::nil();
        } else {
            m_string.truncate(end_idx);
            return this;
        }

        // When called with custom global record separator and no args
        // don't remove \r unless specified by record separator
    } else if (record_separator.is_null()) {
        size_t end_idx = length();
        size_t last_idx = end_idx - 1;
        size_t sep_idx = global_record_separator.length() - 1;

        while (m_string.at(last_idx) == global_record_separator.at(sep_idx)) {
            if (sep_idx == 0) {
                end_idx = last_idx;
                break;
            }

            if (last_idx == 0) {
                break;
            }

            --last_idx;
            --sep_idx;
        }

        if (end_idx == m_string.length()) {
            return Value::nil();
        } else {
            m_string.truncate(end_idx);
            return this;
        }
    }

    record_separator.assert_type(env, Object::Type::String, "String");

    const String rs = record_separator.as_string()->m_string;
    size_t rs_len = rs.length();

    // when passed empty string remove trailing \n and \r\n but not \r
    if (rs_len == 0) {
        while (end_idx > 0 && m_string.at(end_idx - 1) == '\n') {
            if (end_idx > 1 && m_string.at(end_idx - 2) == '\r') {
                --end_idx;
            }
            --end_idx;
        }

        if (end_idx == m_string.length()) {
            return Value::nil();
        } else {
            m_string.truncate(end_idx);
            return this;
        }
    }

    size_t last_idx = end_idx - 1;
    size_t rs_idx = rs_len - 1;

    // remove only final \r when called with (non empty) string on
    // a string terminating in \r
    if (m_string.at(last_idx) == '\r') {
        m_string.truncate(end_idx - 1);
        return this;
    }

    // called with (non empty) string
    while (m_string.at(last_idx) == rs.at(rs_idx)) {
        if (rs_idx == 0) {
            if (last_idx != 0 && m_string.at(last_idx - 1) == '\r') {
                --last_idx;
            }

            end_idx = last_idx;
            break;
        }

        if (last_idx == 0) {
            break;
        }

        --last_idx;
        --rs_idx;
    }

    if (end_idx == m_string.length()) {
        return Value::nil();
    } else {
        m_string.truncate(end_idx);
        return this;
    }
}

Value StringObject::chr(Env *env) {
    if (this->is_empty()) {
        return new StringObject { "", m_encoding };
    }
    size_t index = 0;
    auto c = next_char(&index);
    return new StringObject { c, m_encoding };
}

SymbolObject *StringObject::to_symbol(Env *env) const {
    return SymbolObject::intern(m_string, m_encoding.ptr());
}

Value StringObject::to_sym(Env *env) const {
    return to_symbol(env);
}

Value StringObject::tr(Env *env, Value from_value, Value to_value) const {
    auto copy = new StringObject { m_string, m_encoding };
    copy->tr_in_place(env, from_value, to_value);
    return copy;
}

Value StringObject::tr_in_place(Env *env, Value from_value, Value to_value) {
    assert_not_frozen(env);

    auto from_chars = from_value.to_str(env)->chars(env).as_array_or_raise(env);
    auto to_chars = to_value.to_str(env)->chars(env).as_array_or_raise(env);

    // nothing to do
    if (from_chars->is_empty())
        return Value::nil();

    bool inverted_match = false;
    if (from_chars->size() > 1 && *(from_chars->first().as_string()) == "^") {
        inverted_match = true;
        from_chars->shift();
    }

    // convert ranges ('a-g') into literal characters
    auto expand_ranges = [&](ArrayObject *ary) {
        if (ary->is_empty()) return;

        // (have to iterate backwards so we can insert chars)
        for (ssize_t i = ary->size() - 1; i >= 0; i--) {
            if (i - 2 < 0)
                break;

            auto c1 = ary->at(i - 2).as_string();
            auto sep = ary->at(i - 1).as_string();
            auto c2 = ary->at(i).as_string();

            if (*sep == "-" && *c1 != "-" && *c2 != "-") {
                if (c1->string().cmp(c2->string()) == 1)
                    env->raise("ArgumentError", "invalid range \"{}-{}\" in string transliteration", c1->string(), c2->string());

                auto range = RangeObject::create(env, c1, c2, false);
                auto all_chars = range->to_a(env);
                ary->refeq(env, Value::integer(i - 2), Value::integer(3), all_chars);
            }
        }
    };
    expand_ranges(from_chars);
    expand_ranges(to_chars);

    if (!to_chars->is_empty()) {
        // pad out to_chars to match from_chars size
        while (to_chars->size() < from_chars->size())
            to_chars->push(to_chars->last());
    }

    bool changes_made = false;

    auto replace_char = [&](size_t index, size_t size, size_t replacement_index) {
        if (replacement_index >= to_chars->size()) {
            m_string.replace_bytes(index, size, "");
        } else {
            auto bytes = to_chars->at(replacement_index).as_string()->string();
            m_string.replace_bytes(index, size, bytes);
        }
        changes_made = true;
    };

    if (inverted_match) {
        bool changes_made = false;
        size_t byte_index = length();
        auto c = prev_char(&byte_index);
        while (!c.is_empty()) {
            auto last_char_index = to_chars->size() >= 1 ? to_chars->size() - 1 : 0;

            bool found = false;
            for (size_t j = 0; j < from_chars->size(); j++) {
                if (*from_chars->at(j).as_string() == c) {
                    found = true;
                    break;
                }
            }
            if (!found)
                replace_char(byte_index, c.size(), last_char_index);

            c = prev_char(&byte_index);
        }

    } else { // regular match
        size_t byte_index = length();
        auto c = prev_char(&byte_index);
        while (!c.is_empty()) {
            for (size_t j = 0; j < from_chars->size(); j++) {
                if (*from_chars->at(j).as_string() == c) {
                    replace_char(byte_index, c.size(), j);
                    break;
                }
            }
            c = prev_char(&byte_index);
        }
    }

    if (!changes_made)
        return Value::nil();

    return this;
}

StringObject *StringObject::inspect(Env *env) const {
    String out { "\"" };
    auto encoding = m_encoding.ptr();

    auto utf8_encoding = EncodingObject::get(Encoding::UTF_8);

    size_t index = 0;
    auto [valid, ch] = next_char_result(&index);
    while (!ch.is_empty()) {
        if (!valid) {
            for (size_t i = 0; i < ch.size(); i++)
                out.append_sprintf("\\x%02X", static_cast<uint8_t>(ch[i]));
            auto pair = next_char_result(&index);
            valid = pair.first;
            ch = pair.second;
            continue;
        }
        const auto c = encoding->decode_codepoint(ch);
        auto pair = next_char_result(&index);
        valid = pair.first;
        const auto c2 = !valid || ch.is_empty() ? 0 : encoding->decode_codepoint(pair.second);

        if (c == '"' || c == '\\' || (c == '#' && (c2 == '{' || c2 == '$' || c2 == '@'))) {
            out.append_char('\\');
            out.append_char(c);
        } else if (c == '\a') {
            out.append("\\a");
        } else if (c == '\b') {
            out.append("\\b");
        } else if (c == 27) {
            out.append("\\e");
        } else if (c == '\f') {
            out.append("\\f");
        } else if (c == '\n') {
            out.append("\\n");
        } else if (c == '\r') {
            out.append("\\r");
        } else if (c == '\t') {
            out.append("\\t");
        } else if (c == '\v') {
            out.append("\\v");
        } else if (encoding->is_printable_char(c)) {
            if (encoding == utf8_encoding || c <= 255)
                out.append(utf8_encoding->encode_codepoint(c));
            else
                encoding->append_escaped_char(out, c);
        } else {
            encoding->append_escaped_char(out, c);
        }
        ch = pair.second;
    }

    out.append_char('"');

    return new StringObject { std::move(out) };
}

String StringObject::dbg_inspect() const {
    return String::format("\"{}\"", m_string);
}

StringObject *StringObject::successive(Env *env) {
    auto str = m_string.successive();
    return new StringObject { str, m_encoding };
}

StringObject *StringObject::successive_in_place(Env *env) {
    assert_not_frozen(env);
    m_string = m_string.successive();
    return this;
}

StringObject *StringObject::to_s() {
    if (m_klass == GlobalEnv::the()->String()) {
        return this;
    } else {
        return new StringObject { m_string, m_encoding };
    }
}

bool StringObject::internal_start_with(Env *env, Value needle) {
    if (needle.is_regexp()) {
        needle = needle.as_regexp()->to_s(env);
        needle.as_string()->prepend(env, { new StringObject { "\\A" } });
        needle = new RegexpObject { env, needle.as_string()->string() };
        return needle.as_regexp()->match(env, this, nullptr).is_truthy();
    }

    nat_int_t i = index_int(env, needle, 0);
    return i == 0;
}

bool StringObject::start_with(Env *env, Args &&args) {
    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = args[i];

        if (internal_start_with(env, arg))
            return true;
    }

    return false;
}

// NATFIXME : broken for searching the middle of a multibyte char
bool StringObject::end_with(Env *env, Value needle) const {
    needle = needle.to_str(env);
    needle.assert_type(env, Object::Type::String, "String");
    if (length() < needle.as_string()->length())
        return false;
    auto from_end = new StringObject { c_str() + length() - needle.as_string()->length() };
    nat_int_t i = from_end->index_int(env, needle, 0);
    return i == 0;
}

bool StringObject::end_with(Env *env, Args &&args) const {
    for (size_t i = 0; i < args.size(); ++i) {
        if (end_with(env, args[i]))
            return true;
    }
    return false;
}

static Value byteindex_regexp_needle(Env *env, const StringObject *haystack, RegexpObject *needle, OnigPosition offset, bool reverse = false) {
    if (!haystack->negotiate_compatible_encoding(needle->pattern())) {
        auto exception_class = fetch_nested_const({ "Encoding"_s, "CompatibilityError"_s }).as_class();
        auto enc1 = needle->pattern()->encoding()->name()->string();
        auto enc2 = haystack->encoding()->name()->string();
        env->raise(exception_class, "incompatible encoding regexp match ({} regexp with {} string)", enc1, enc2);
    }

    if (needle->pattern()->is_empty())
        return Value::integer(offset);

    OnigRegion *region = onig_region_new();

    int result = needle->search(env, haystack, offset, region, ONIG_OPTION_NONE, reverse);

    if (result == ONIG_MISMATCH) {
        onig_region_free(region, true);
        env->caller()->set_last_match(nullptr);
        return Value::nil();
    }

    auto match = new MatchDataObject { region, haystack, needle };
    env->caller()->set_last_match(match);
    auto byte_index = region->beg[0];
    return Value::integer(byte_index);
}

static Value byteindex_string_needle(Env *env, const StringObject *haystack, StringObject *needle_obj, size_t offset, bool reverse = false) {
    haystack->assert_compatible_string(env, needle_obj);
    String needle = needle_obj->string();

    if (reverse) {
        if ((size_t)offset > haystack->bytesize())
            return Value::nil();
    } else {
        if ((size_t)offset + needle.size() > haystack->bytesize())
            return Value::nil();
    }

    if ((size_t)offset < haystack->bytesize() && !haystack->encoding()->is_valid_codepoint_boundary(haystack->string(), offset))
        env->raise("IndexError", "offset {} does not land on character boundary", offset);

    if (needle.is_empty())
        return Value::integer(offset);

    if (!reverse && (size_t)offset >= haystack->bytesize())
        return Value::nil();

    void *pointer = nullptr;
    if (reverse) {
        if (offset + needle.size() >= haystack->bytesize())
            offset = haystack->bytesize() - needle.size();
        for (ssize_t i = offset; i >= 0; i--) {
            if (memcmp(haystack->c_str() + i, needle.c_str(), needle.size()) == 0) {
                pointer = (void *)(haystack->c_str() + i);
                break;
            }
        }
    } else {
        pointer = memmem(haystack->c_str() + offset, haystack->bytesize() - offset, needle.c_str(), needle.size());
    }

    if (!pointer)
        return Value::nil();

    auto result = (const char *)pointer - haystack->c_str();
    return Value::integer(result);
}

Value StringObject::byteindex(Env *env, Value needle_obj, Value offset_obj) const {
    ssize_t offset = 0;
    if (offset_obj)
        offset = IntegerMethods::convert_to_native_type<ssize_t>(env, offset_obj);
    if (offset < 0)
        offset += bytesize();
    if (offset < 0 || (size_t)offset > bytesize())
        return Value::nil();

    if (needle_obj.is_regexp())
        return byteindex_regexp_needle(env, this, needle_obj.as_regexp(), offset);

    auto needle = needle_obj.to_str2(env);
    return byteindex_string_needle(env, this, needle, offset);
}

Value StringObject::byterindex(Env *env, Value needle_obj, Value offset_obj) const {
    ssize_t offset = bytesize();
    if (offset_obj)
        offset = IntegerMethods::convert_to_native_type<ssize_t>(env, offset_obj);
    if (offset < 0)
        offset += bytesize();
    if (offset < 0)
        return Value::nil();
    offset = std::min((size_t)offset, bytesize());

    if (needle_obj.is_regexp())
        return byteindex_regexp_needle(env, this, needle_obj.as_regexp(), offset, true);

    auto needle = needle_obj.to_str2(env);
    return byteindex_string_needle(env, this, needle, offset, true);
}

Value StringObject::index(Env *env, Value needle, Value offset) {
    int offset_i = (offset) ? IntegerMethods::convert_to_int(env, offset) : 0;
    int len = char_count(env);
    if (offset_i < -1 * len) {
        // extremely negative offset larger than string length returns nil
        return Value::nil();
    } else if (offset_i < 0) {
        // negative offset adds to string length
        offset_i += len;
    }
    return index(env, needle, ::abs(offset_i));
}

Value StringObject::index(Env *env, Value needle, size_t start) {
    auto byte_start = char_index_to_byte_index(start);
    if (byte_start == -1)
        return Value::nil();
    auto byte_index = index_int(env, needle, byte_start);
    if (byte_index == -1)
        return Value::nil();
    auto char_index = byte_index_to_char_index(byte_index);
    return IntegerMethods::from_size_t(env, char_index);
}

nat_int_t StringObject::index_int(Env *env, Value needle, size_t byte_start) {
    if (needle.is_regexp()) {
        // FIXME: use byteindex_regexp_needle shared code
        if (needle.as_regexp()->pattern()->is_empty())
            return byte_start;

        if (bytesize() == 0)
            return -1;

        if (byte_start >= bytesize())
            return -1;

        OnigRegion *region = onig_region_new();
        int result = needle.as_regexp()->search(env, this, byte_start, region, ONIG_OPTION_NONE);
        if (result == ONIG_MISMATCH) {
            onig_region_free(region, true);
            env->caller()->set_last_match(nullptr);
            return -1;
        }

        auto match = new MatchDataObject { region, this, needle.as_regexp() };
        env->caller()->set_last_match(match);
        return region->beg[0];
    }

    auto needle_str = needle.to_str(env);
    assert_compatible_string(env, needle_str);

    if (needle_str->bytesize() == 0)
        return byte_start;

    if (bytesize() == 0)
        return -1;

    if (byte_start >= bytesize())
        return -1;

    if (needle_str->bytesize() > bytesize() - byte_start)
        return -1;

    auto ptr = memmem(c_str() + byte_start, bytesize() - byte_start, needle_str->c_str(), needle_str->bytesize());
    if (ptr == nullptr)
        return -1;

    return (char *)ptr - c_str();
}

Value StringObject::rindex(Env *env, Value needle, Value offset) const {
    int len = char_count(env);
    int offset_i = (offset) ? IntegerMethods::convert_to_int(env, offset) : len;
    if (offset_i < -1 * len) {
        // extremely negative offset larger than string length returns nil
        return Value::nil();
    } else if (offset_i < 0) {
        // negative offset adds to string length
        offset_i += len;
    } else if (offset_i > len) {
        offset_i = len;
    }
    return rindex(env, needle, ::abs(offset_i));
}

Value StringObject::rindex(Env *env, Value needle, size_t start) const {
    auto byte_start = char_index_to_byte_index(start);
    if (byte_start == -1)
        return Value::nil();
    auto byte_index = rindex_int(env, needle, byte_start);
    if (byte_index == -1)
        return Value::nil();
    auto char_index = byte_index_to_char_index(byte_index);
    return IntegerMethods::from_size_t(env, char_index);
}

nat_int_t StringObject::rindex_int(Env *env, Value needle, size_t byte_start) const {
    if (needle.is_regexp()) {
        // FIXME: use byteindex_regexp_needle shared code

        auto needle_regexp = needle.as_regexp();
        if (!negotiate_compatible_encoding(needle_regexp->pattern())) {
            auto exception_class = fetch_nested_const({ "Encoding"_s, "CompatibilityError"_s }).as_class();
            auto enc1 = needle_regexp->encoding()->name()->string();
            auto enc2 = encoding()->name()->string();
            env->raise(exception_class, "incompatible encoding regexp match ({} regexp with {} string)", enc1, enc2);
        }

        if (needle.as_regexp()->pattern()->is_empty())
            return byte_start;

        if (bytesize() == 0)
            return -1;

        if (byte_start > bytesize())
            return -1;

        OnigRegion *region = onig_region_new();
        int result = needle_regexp->search(env, this, byte_start, region, ONIG_OPTION_NONE, true);
        if (result == ONIG_MISMATCH) {
            onig_region_free(region, true);
            env->caller()->set_last_match(nullptr);
            return -1;
        }

        auto match = new MatchDataObject { region, this, needle_regexp };
        env->caller()->set_last_match(match);
        return region->beg[0];
    }

    auto needle_str = needle.to_str(env);
    assert_compatible_string(env, needle_str);

    if (needle_str->bytesize() == 0)
        return byte_start;

    if (bytesize() == 0)
        return -1;

    if (byte_start > bytesize())
        return -1;

    auto byte_index = bytesize();
    auto c = prev_char(&byte_index);
    auto needle_len = needle_str->length();
    while (!c.is_empty()) {
        if (byte_index <= byte_start && needle_len <= bytesize() - byte_index && memcmp(needle_str->c_str(), c_str() + byte_index, needle_len) == 0) {
            return byte_index;
        }

        c = prev_char(&byte_index);
    }

    return -1;
}

Value StringObject::initialize(Env *env, Value arg, Value encoding, Value capacity) {
    if (arg)
        initialize_copy(env, arg.to_str(env));
    if (encoding) {
        force_encoding(env, encoding);
    }
    if (capacity) {
        const auto cap = IntegerMethods::convert_to_native_type<size_t>(env, capacity);
        m_string.set_capacity(cap);
    }
    return this;
}

Value StringObject::initialize_copy(Env *env, Value arg) {
    assert_not_frozen(env);
    auto string_obj = arg.to_str(env);
    m_string = string_obj->string();
    m_encoding = string_obj->encoding();
    return this;
}

Value StringObject::ltlt(Env *env, Value arg) {
    concat(env, { arg });
    return this;
}

Value StringObject::add(Env *env, Value arg) const {
    StringObject *str = arg.to_str(env);
    StringObject *new_string = new StringObject { m_string, m_encoding };
    Value args[] = { str };
    new_string->concat(env, Args(1, args));
    return new_string;
}

Value StringObject::append_as_bytes(Env *env, Args &&args) {
    assert_not_frozen(env);
    String buf;
    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];
        if (arg.is_string()) {
            buf.append(arg.as_string()->string());
        } else if (arg.is_integer()) {
            const auto c = static_cast<uint8_t>(arg.send(env, "&"_s, { Value::integer(0xFF) }).integer().to_nat_int_t());
            buf.append_char(c);
        } else {
            env->raise("TypeError", "wrong argument type {} (expected String or Integer)", arg.klass()->inspect_str());
        }
    }
    m_string.append(buf);
    return this;
}

Value StringObject::mul(Env *env, Value arg) const {
    auto int_arg = arg.to_int(env);
    if (int_arg.is_negative())
        env->raise("ArgumentError", "negative argument");

    auto nat_int = IntegerMethods::convert_to_nat_int_t(env, int_arg);
    if (nat_int && (std::numeric_limits<size_t>::max() / nat_int) < length())
        env->raise("ArgumentError", "argument too big");

    String new_string { length() * nat_int, '\0' };
    if (!is_empty()) {
        for (nat_int_t i = 0; i < nat_int; i++)
            new_string.replace_bytes(i * length(), length(), string());
    }
    return new StringObject { std::move(new_string), m_encoding };
}

Value StringObject::clear(Env *env) {
    assert_not_frozen(env);

    m_string.clear();

    return this;
}

Value StringObject::cmp(Env *env, Value other) {
    StringObject *other_str;
    if (other.is_string()) {
        other_str = other.as_string();
    } else if (other.respond_to(env, "to_str"_s)) {
        other_str = other.to_str(env);
    } else if (other.respond_to(env, "<=>"_s)) {
        auto negative_cmp = other.send(env, "<=>"_s, { this });
        if (negative_cmp.is_nil()) {
            return negative_cmp;
        }
        auto i = negative_cmp.to_int(env);
        return -i;
    } else {
        return Value::nil();
    }

    if (m_string.is_empty() && other_str->m_string.is_empty())
        return Value::integer(0);

    auto comparison = m_string.cmp(other_str->m_string);

    if (comparison == 0 && !(is_ascii_only() && other_str->is_ascii_only())) {
        nat_int_t this_enc_idx = static_cast<nat_int_t>(m_encoding->num());
        nat_int_t other_enc_idx = static_cast<nat_int_t>(other_str->m_encoding->num());
        nat_int_t cmp_encodings = this_enc_idx - other_enc_idx;
        if (cmp_encodings > 0)
            return Value::integer(1);
        else if (cmp_encodings == 0)
            return Value::integer(0);
        else
            return Value::integer(-1);
    }

    return Value::integer(comparison);
}

Value StringObject::concat(Env *env, Args &&args) {
    assert_not_frozen(env);

    StringObject *original = new StringObject(*this);

    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        StringObject *str_obj;
        if (arg.is_string()) {
            str_obj = arg.as_string();
        } else if (arg.is_integer() && arg.integer().is_negative()) {
            env->raise("RangeError", "{} out of char range", arg.integer().to_string());
        } else if (arg.is_integer()) {
            // Special case: US-ASCII << (128..255) will change the string to binary
            if (m_encoding == EncodingObject::get(Encoding::US_ASCII) && arg.integer().is_fixnum()) {
                const auto nat_int = arg.integer();
                if (nat_int >= 128 && nat_int <= 255)
                    m_encoding = EncodingObject::get(Encoding::ASCII_8BIT);
            }
            str_obj = arg.send(env, "chr"_s, { m_encoding.ptr() }).as_string();
        } else {
            str_obj = arg.to_str(env);
        }

        Value(str_obj).assert_type(env, Object::Type::String, "String");

        // If the other string is empty, there's nothing to do.
        if (str_obj->is_empty()) continue;

        // If this string is empty, then we can just copy the other string and encoding.
        if (is_empty()) {
            m_string = str_obj->string();
            m_encoding = str_obj->encoding();
            continue;
        }

        assert_compatible_string_and_update_encoding(env, str_obj);

        append(str_obj->string());
    }

    return this;
}

Value StringObject::count(Env *env, Args &&args) {
    nat_int_t total_count = 0;
    auto handler = character_class_handler(env, std::move(args));
    for (auto character : *this) {
        if (handler(character))
            total_count++;
    }
    return Value::integer(total_count);
}

Value StringObject::crypt(Env *env, Value salt) {
    for (const auto c : *this) {
        if (c == '\0')
            env->raise("ArgumentError", "string contains null byte");
    }

    const auto salt_str = salt.to_str(env);

    // Use the null terminated length of the string
    if (strlen(salt_str->c_str()) < 2)
        env->raise("ArgumentError", "salt too short (need >=2 bytes)");

    auto crypted = ::crypt(c_str(), salt_str->c_str());
    if (!crypted)
        env->raise_errno();

    return new StringObject { crypted };
}

Value StringObject::delete_str(Env *env, Args &&selectors) {
    auto dup = new StringObject { m_string, m_encoding };
    auto result = dup->delete_in_place(env, std::move(selectors));
    if (result.is_nil())
        return dup;
    return result;
}

Value StringObject::delete_in_place(Env *env, Args &&selectors) {
    assert_not_frozen(env);
    const auto old_len = bytesize();
    auto handler = character_class_handler(env, std::move(selectors));
    size_t index = 0;
    while (index < m_string.size()) {
        const auto old_index = index;
        auto character = next_char(&index);
        if (handler(character)) {
            m_string.replace_bytes(old_index, character.size(), "");
            index = old_index;
        }
    }
    if (bytesize() == old_len)
        return Value::nil();
    return this;
}

bool StringObject::eq(Env *env, Value arg) {
    if (!arg.is_string() && arg.respond_to(env, "to_str"_s))
        return arg.send(env, "=="_s, { this });
    return eql(arg);
}

Value StringObject::eqtilde(Env *env, Value other) {
    if (other.is_string())
        env->raise("TypeError", "type mismatch: String given");

    if (!other.is_regexp() && other.respond_to(env, "=~"_s))
        return other.send(env, "=~"_s, { this });

    other.assert_type(env, Object::Type::Regexp, "Regexp");
    return other.as_regexp()->eqtilde(env, this);
}

Value StringObject::match(Env *env, Value other, Value index, Block *block) {
    if (!other.is_regexp()) {
        if (other.is_string()) {
            other = new RegexpObject { env, other.as_string()->string() };
        } else if (other.respond_to(env, "to_str"_s)) {
            other = new RegexpObject { env, other.to_str(env)->string() };
        } else if (other.respond_to(env, "=~"_s)) {
            return other.send(env, "=~"_s, { this });
        }
    }
    other.assert_type(env, Object::Type::Regexp, "Regexp");
    auto result = other.send(env, "match"_s, { this, index }, block);
    env->caller()->set_match(env->match());
    return result;
}

Value StringObject::ord(Env *env) const {
    size_t index = 0;
    auto result = next_char_result(&index);
    if (!result.first)
        env->raise_invalid_byte_sequence_error(m_encoding.ptr());
    auto c = result.second;
    if (c.is_empty())
        env->raise("ArgumentError", "empty string");
    auto code = m_encoding->decode_codepoint(c);
    if (code == -1)
        env->raise_invalid_byte_sequence_error(m_encoding.ptr());
    return Value::integer(code);
}

Value StringObject::prepend(Env *env, Args &&args) {
    assert_not_frozen(env);

    StringObject *original = new StringObject(*this);

    String appendable;
    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        StringObject *str_obj;
        if (arg.is_string()) {
            str_obj = arg.as_string();
        } else if (arg.is_integer() && arg.integer() < 0) {
            env->raise("RangeError", "{} out of char range", arg.integer().to_string());
        } else if (arg.is_integer()) {
            str_obj = arg.send(env, "chr"_s, { m_encoding.ptr() }).as_string();
        } else {
            str_obj = arg.to_str(env);
        }

        Value(str_obj).assert_type(env, Object::Type::String, "String");
        appendable.append(&str_obj->m_string);
    }
    m_string.prepend(&appendable);

    return this;
}

Value StringObject::b(Env *env) const {
    return new StringObject { m_string, Encoding::ASCII_8BIT };
}

Value StringObject::bytes(Env *env, Block *block) {
    if (block) {
        return each_byte(env, block);
    }
    ArrayObject *ary = new ArrayObject { length() };
    for (size_t i = 0; i < length(); ++i) {
        unsigned char c = m_string[i];
        ary->push(Value::integer(c));
    }
    return ary;
}

Value StringObject::each_byte(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { *env, this, StringObject::bytesize_fn, 0 };
        return send(env, "enum_for"_s, { "each_byte"_s }, size_block);
    }

    for (size_t i = 0; i < length(); i++) {
        unsigned char c = c_str()[i];
        Value args[] = { Value::integer(c) };
        block->run(env, Args(1, args), nullptr);
    }
    return this;
}

size_t StringObject::char_count(Env *env) const {
    if (m_encoding->is_single_byte_encoding())
        return bytesize();

    size_t index = 0;
    size_t char_count = 0;
    auto c = next_char(&index);
    while (!c.is_empty()) {
        char_count++;
        c = next_char(&index);
    }
    return char_count;
}

Value StringObject::scan(Env *env, Value pattern, Block *block) {
    if (!pattern.is_regexp())
        pattern = RegexpObject::compile(env, RegexpObject::quote(env, pattern.to_str(env)));
    pattern.assert_type(env, Type::Regexp, "Regexp");

    auto regexp = pattern.as_regexp();
    auto ary = new ArrayObject {};
    size_t byte_index = 0;
    size_t new_byte_index = 0;
    size_t total_size = m_string.size();
    Value match_value = nullptr;
    MatchDataObject *match_obj = nullptr;

    auto caller_env = env->caller();

    while (!(match_value = regexp->match_at_byte_offset(env, this, byte_index)).is_nil()) {
        match_obj = match_value.as_match_data();
        env->set_match(match_obj);

        if (match_obj->has_captures()) {
            auto captures = match_obj->captures(env).as_array_or_raise(env);
            if (block) {
                Value args[] = { captures };
                block->run(env, Args(1, args), nullptr);
            } else {
                ary->push(captures);
            }
        } else {
            auto str = match_obj->to_s(env);
            if (block) {
                Value args[] = { str };
                block->run(env, Args(1, args), nullptr);
            } else {
                ary->push(str);
            }
        }

        if (match_obj->is_empty()) {
            // To match MRI String#scan semantics, we need to increment
            // by a character every time there is an empty match...
            auto result = next_char(&byte_index);
            // ...and one extra when we reach the end of the string.
            if (result.is_empty()) byte_index++;
        } else {
            byte_index = match_obj->end_byte_index(0);
        }

        if (byte_index > total_size)
            break;
    }

    caller_env->set_last_match(match_obj);

    if (block)
        return this;

    return ary;
}

Value StringObject::setbyte(Env *env, Value index_obj, Value value_obj) {
    assert_not_frozen(env);

    nat_int_t index = IntegerMethods::convert_to_nat_int_t(env, index_obj);
    nat_int_t value = IntegerMethods::convert_to_nat_int_t(env, value_obj);
    nat_int_t length = static_cast<nat_int_t>(m_string.length());
    nat_int_t index_original = index;

    if (index < 0) {
        index += length;
    }

    if (index < 0) {
        env->raise("IndexError", "index {} out of string", index_original);
    }

    if (index >= length) {
        env->raise("IndexError", "index {} out of string", index_original);
    }

    m_string[index] = value % 256;
    return value_obj;
}

Value StringObject::size(Env *env) const {
    return IntegerMethods::from_size_t(env, char_count(env));
}

Value StringObject::encode(Env *env, Value dst_encoding, Value src_encoding, HashObject *kwargs) {
    StringObject *copy = duplicate(env).as_string();
    return copy->encode_in_place(env, dst_encoding, src_encoding, kwargs);
}

Value StringObject::encode_in_place(Env *env, Value dst_encoding, Value src_encoding, HashObject *kwargs) {
    assert_not_frozen(env);

    if (!dst_encoding)
        dst_encoding = EncodingObject::default_internal();
    if (!dst_encoding)
        dst_encoding = EncodingObject::get(Encoding::UTF_8);

    if (!src_encoding)
        src_encoding = m_encoding.ptr();

    EncodeOptions options;
    if (kwargs) {
        if (kwargs->remove(env, "universal_newline"_s))
            options.newline_option = EncodeNewlineOption::Universal;
        else if (kwargs->remove(env, "crlf_newline"_s))
            options.newline_option = EncodeNewlineOption::Crlf;
        else if (kwargs->remove(env, "cr_newline"_s))
            options.newline_option = EncodeNewlineOption::Cr;

        auto invalid = kwargs->remove(env, "invalid"_s);
        if (invalid) {
            if (invalid.is_nil())
                options.invalid_option = EncodeInvalidOption::Raise;
            else if (invalid == "replace"_s)
                options.invalid_option = EncodeInvalidOption::Replace;
        }

        auto undef = kwargs->remove(env, "undef"_s);
        if (undef) {
            if (undef.is_nil())
                options.undef_option = EncodeUndefOption::Raise;
            else if (undef == "replace"_s)
                options.undef_option = EncodeUndefOption::Replace;
        }

        auto replace = kwargs->remove(env, "replace"_s);
        if (replace && !replace.is_nil())
            options.replace_option = replace.as_string_or_raise(env)->encode(env, dst_encoding).as_string_or_raise(env);

        auto fallback = kwargs->remove(env, "fallback"_s);
        if (fallback && !fallback.is_nil())
            options.fallback_option = fallback;

        auto xml = kwargs->remove(env, "xml"_s);
        if (xml) {
            if (xml == "attr"_s)
                options.xml_option = EncodeXmlOption::Attr;
            else if (xml == "text"_s)
                options.xml_option = EncodeXmlOption::Text;
            else
                env->raise("ArgumentError", "unexpected value for xml option: {}", xml.inspect_str(env));
        }
    }

    auto find_encoding = [&](Value encoding) {
        if (encoding.is_encoding())
            return encoding.as_encoding();

        auto name = encoding.to_str(env)->string();
        return EncodingObject::find_encoding_by_name(env, name);
    };

    env->ensure_no_extra_keywords(kwargs);
    EncodingObject *dst_encoding_obj = find_encoding(dst_encoding);
    EncodingObject *src_encoding_obj = find_encoding(src_encoding);
    if (!dst_encoding_obj || !src_encoding_obj) {
        auto klass = m_encoding->klass()->const_find(env, "ConverterNotFoundError"_s).as_class();
        auto to_name = dst_encoding.to_s(env)->string();
        auto from_name = src_encoding.to_s(env)->string();
        env->raise(klass, "code converter not found ({} to {})", from_name, to_name);
    }

    return dst_encoding_obj->encode(env, src_encoding_obj, this, options);
}

Value StringObject::force_encoding(Env *env, Value encoding) {
    assert_not_frozen(env);
    set_encoding(EncodingObject::find_encoding(env, encoding));
    return this;
}

bool StringObject::has_match(Env *env, Value other, Value start) {
    other.assert_type(env, Object::Type::Regexp, "Regexp");
    return other.as_regexp()->has_match(env, this, start);
}

/**
 * String#hex
 *
 * Converts the string to a base 16 integer. Includes an optional type, as well
 * as allowing an optional 0x prefix.
 *
 * To implement this, we effectively run the string through a state machine. The
 * state machine looks approximately like this:
 *
 *         +------------------- 0x ------------------+
 *         |                                         V
 *     +-------+            +------+           +--------+
 * --> | start | -- +/- --> | sign | -- 0x --> | prefix |
 *     +-------+            +------+           +--------+
 *         |                  |                      |
 *         |                  \h  +------- \h -------+
 *         |                  V   V
 *         |                +--------+ --- \h ---+
 *         +---- \h ------> | number |           |
 *                          +--------+ <---------+
 *                           |     ^
 *                           _     \h
 *                           V     |
 *                          +------------+
 *                          | underscore |
 *                          +------------+
 */
Value StringObject::hex(Env *env) const {
    // This is an enum that represents the states of the state machine.
    enum {
        start,
        sign,
        prefix,
        number,
        underscore,
    } state
        = start;

    bool negative = false;
    nat_int_t value = 0;

    const char *str_source = c_str();
    size_t str_length = length();

    for (size_t index = 0; index < str_length; index++) {
        char c = str_source[index];
        int char_value;

        switch (state) {
        case start:
            if (c == '-') {
                negative = true;
                state = sign;
            } else if (c == '+') {
                state = sign;
            } else if (c == '0' && index + 1 < str_length && str_source[index + 1] == 'x') {
                state = prefix;
                index++;
            } else if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = char_value;
                state = number;
            } else {
                return Value::integer(0);
            }
            break;
        case sign:
            if (c == '0' && index + 1 < str_length && str_source[index + 1] == 'x') {
                state = prefix;
                index++;
            } else if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = char_value;
                state = number;
            } else {
                return Value::integer(0);
            }
            break;
        case prefix:
            if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = char_value;
                state = number;
            } else {
                return Value::integer(0);
            }
            break;
        case number:
            if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = (value * 16) + char_value;
            } else if (c == '_') {
                state = underscore;
            } else {
                return Value::integer((negative ? -value : value));
            }
            break;
        case underscore:
            if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = (value * 16) + char_value;
                state = number;
            } else {
                return Value::integer((negative ? -value : value));
            }
            break;
        }
    }

    return Value::integer((negative ? -value : value));
}

/**
 * String#[] and String#slice
 *
 * These methods can be called with a variety of call signatures. They include:
 *
 * slice(index) → new_str or nil
 * slice(start, length) → new_str or nil
 * slice(range) → new_str or nil
 * slice(regexp) → new_str or nil
 * slice(regexp, capture) → new_str or nil
 * slice(match_str) → new_str or nil
 */
Value StringObject::ref(Env *env, Value index_obj, Value length_obj) {
    // not sure how we'd handle a string that big anyway
    assert(length() < NAT_INT_MAX);

    // First, check if there's a second argument. This is important for the
    // order in which things are attempted to be implicitly converted into
    // integers.
    if (length_obj != nullptr) {
        // First, we'll check if the index is a regexp. If it's not and there
        // _is_ a second argument, the index is assumed to be an integer or an
        // object that can be implicitly converted into an integer.
        if (index_obj.is_regexp()) {
            auto regexp = index_obj.as_regexp();
            auto match_result = regexp->match(env, this);

            // The second argument can be either an index into the match data's
            // captures or the name of a group. If it's not a string, make sure
            // we attempt to convert it into an integer _before_ we return nil
            // if there we no match result.
            if (!length_obj.is_string())
                IntegerMethods::convert_to_nat_int_t(env, length_obj);

            // If the match failed, return nil. Note that this must happen after
            // the implicit conversion of the index argument to an integer.
            if (match_result.is_nil())
                return match_result;

            // Otherwise, return the region of the string that was captured
            // indicated by the second argument.
            return match_result.as_match_data()->ref(env, length_obj);
        } else {
            // First, attempt to convert the index object into an integer, and
            // make sure it fits into a fixnum.
            nat_int_t index = IntegerMethods::convert_to_nat_int_t(env, index_obj);

            // If we have a length, then attempt to convert the length object
            // into an integer, and make sure it fits into a fixnum.
            nat_int_t length = IntegerMethods::convert_to_nat_int_t(env, length_obj);
            nat_int_t count = static_cast<nat_int_t>(char_count(env));

            // Now, check that the index is within the bounds of the string. If
            // not, then return nil.
            if (length < 0 || index > count || index + count < 0)
                return Value::nil();

            // Shortcut here before doing anything else to return an empty
            // string if the index is right at the end of the string.
            if (index == count)
                return new StringObject { "", m_encoding };

            // If the index is negative, then we know at this point that it's
            // only negative within one length of the string. So just make it
            // positive here.
            if (index < 0)
                index += count;

            nat_int_t end = index + length > count ? count : index + length;
            return ref_fast_range(env, index, end);
        }
    }

    if (index_obj.is_range()) {
        RangeObject *range = index_obj.as_range();
        auto begin_obj = range->begin();
        auto end_obj = range->end();

        // First, we're going to get the beginning from the range. If it's nil,
        // it's going to be treated as 0. If it's not an integer, it's going to
        // be implicitly converted. Finally, we'll assert that it fits into a
        // fixnum.
        nat_int_t begin;
        if (begin_obj.is_nil()) {
            begin = 0;
        } else {
            begin = IntegerMethods::convert_to_nat_int_t(env, begin_obj);
        }

        // Shortcut here before doing anything else to return an empty
        // string if the index is right at the end of the string.
        auto count = static_cast<nat_int_t>(char_count(env));
        if (begin == count)
            return new StringObject { "", m_encoding };

        // Now, we're going to do some bounds checks on the beginning of the
        // range. If it's too far outside, we'll return nil.
        if (begin + count < 0 || begin > count)
            return Value::nil();

        // Now, if the beginning is negative, we know it's within one length of
        // the string, so we'll add that here to make it a valid positive index.
        if (begin < 0)
            begin += count;

        // Now, we're going to shortcut here if the range is endless since we
        // already have all of the information we need.
        if (end_obj.is_nil())
            return ref_fast_range_endless(env, begin);

        // If it's not endless, then we'll go ahead and grab the ending now by
        // converting to an integer and asserting it fits into a fixnum.
        nat_int_t end = IntegerMethods::convert_to_nat_int_t(env, end_obj);

        // Now, we're going to do some bounds checks on the ending of the range.
        // If it's too negative, we'll return nil.
        if (end + count < 0)
            return new StringObject { "", m_encoding };

        // If the ending is negative, we know it's within one length of the
        // string, so we'll add that here to make it a valid positive index.
        if (end < 0)
            end += count;

        // After we've done the bounds checks and the negative fixes, we can
        // include the fact that it may be inclusive or not.
        if (!range->exclude_end())
            end++;

        // Finally, we can call into our fast range method to get the value.
        return ref_fast_range(env, begin, end);
    } else if (index_obj.is_regexp()) {
        // If the index object is a regular expression, then we'll return the
        // matched substring if there is one. Otherwise, we'll return nil.
        auto regexp = index_obj.as_regexp();
        auto match_result = regexp->match(env, this);

        // If the match failed, return nil.
        if (match_result.is_nil())
            return match_result;

        // Otherwise, return the region of the string that matched.
        return match_result.as_match_data()->to_s(env);
    } else if (index_obj.is_string()) {
        // If the index object is a string, then we return the string if it is
        // found as a substring of this string.
        if (m_string.find(index_obj.as_string()->m_string) != -1)
            return new StringObject { index_obj.as_string()->m_string, index_obj.as_string()->m_encoding };

        // Otherwise we return nil.
        return Value::nil();
    } else {
        // First, attempt to convert the index object into an integer, and
        // make sure it fits into a fixnum.
        nat_int_t index = IntegerMethods::convert_to_nat_int_t(env, index_obj);
        nat_int_t count = static_cast<nat_int_t>(char_count(env));

        // Now, check that the index is within the bounds of the string. If
        // not, then return nil.
        if (index > count || index + count < 0)
            return Value::nil();

        // If the index is negative, then we know at this point that it's
        // only negative within one length of the string. So just make it
        // positive here.
        if (index < 0)
            index += count;

        if (length_obj != nullptr) {
            // If we have a length, then attempt to convert the length object
            // into an integer, and make sure it fits into a fixnum.
            nat_int_t length = IntegerMethods::convert_to_nat_int_t(env, length_obj);

            // Now, check that the index is within the bounds of the string. If
            // not, then return nil.
            if (length < 0)
                return Value::nil();

            // Shortcut here before doing anything else to return an empty
            // string if the index is right at the end of the string.
            if (index == count)
                return new StringObject { "", m_encoding };

            nat_int_t end = index + length > count ? count : index + length;
            return ref_fast_range(env, index, end);
        }

        // Finally, access the index in the string.
        return ref_fast_index(env, index);
    }
}

/**
 * String#byteslice
 *
 * byteslice(index, length = 1) → string or nil
 * byteslice(range) → string or nil
 */
Value StringObject::byteslice(Env *env, Value index_obj, Value length_obj) {
    nat_int_t m_length = static_cast<nat_int_t>(length());

    // not sure how we'd handle a string that big anyway
    assert(m_length < NAT_INT_MAX);

    if (length_obj != nullptr) {
        // First, we're going to get the start index of the slice and make sure
        // that it's not right at the end of the string. If it is, we'll just
        // return an empty string.
        nat_int_t index = IntegerMethods::convert_to_nat_int_t(env, index_obj);
        if (index == m_length)
            return new StringObject("", m_encoding);

        // Next, we're going to get the length that was passed in and make sure
        // that it's not negative. If it is, or the index is too far out of
        // bounds, we'll return nil.
        nat_int_t length = IntegerMethods::convert_to_nat_int_t(env, length_obj);
        nat_int_t ignored;
        if (length < 0 || __builtin_add_overflow(index, m_length, &ignored) || index + m_length < 0 || index > m_length)
            return Value::nil();

        // Next, we'll add the length of the string to the index if it's
        // negative. We know it's within one length of the string because of our
        // previous bounds check. This handles negative indices like -1.
        if (index < 0)
            index += m_length;

        // Finally, we'll determine the length of the substring and create a new
        // string with those bounds.
        nat_int_t s_length = index + length >= m_length ? m_length - index : length;
        return new StringObject { m_string.substring(index, s_length), m_encoding };
    }

    if (index_obj.is_range()) {
        RangeObject *range = index_obj.as_range();
        auto begin_obj = range->begin();
        auto end_obj = range->end();

        // First, we're going to get the beginning from the range. If it's nil,
        // it's going to be treated as 0. If it's not an integer, it's going to
        // be implicitly converted. Finally, we'll assert that it fits into a
        // fixnum.
        nat_int_t begin;
        if (begin_obj.is_nil()) {
            begin = 0;
        } else {
            begin = IntegerMethods::convert_to_nat_int_t(env, begin_obj);
        }

        // Shortcut here before doing anything else to return an empty
        // string if the index is right at the end of the string.
        if (begin == m_length)
            return new StringObject { "", m_encoding };

        // Now, we're going to do some bounds checks on the beginning of the
        // range. If it's too far outside, we'll return nil.
        if (begin + m_length < 0 || begin >= m_length)
            return Value::nil();

        // Now, if the beginning is negative, we know it's within one length of
        // the string, so we'll add that here to make it a valid positive index.
        if (begin < 0)
            begin += m_length;

        // Now, we're going to shortcut here if the range is endless since we
        // already have all of the information we need.
        if (end_obj.is_nil())
            return new StringObject { m_string.substring(begin), m_encoding };

        // If it's not endless, then we'll go ahead and grab the ending now by
        // converting to an integer and asserting it fits into a fixnum.
        nat_int_t end = IntegerMethods::convert_to_nat_int_t(env, end_obj);

        // Now, we're going to do some bounds checks on the ending of the range.
        // If it's too negative, we'll return nil.
        if (end + m_length < 0)
            return new StringObject { "", m_encoding };

        // If the ending is negative, we know it's within one length of the
        // string, so we'll add that here to make it a valid positive index.
        if (end < 0)
            end += m_length;

        // After we've done the bounds checks and the negative fixes, we can
        // include the fact that it may be inclusive or not.
        if (!range->exclude_end())
            end++;

        // Make sure we have a valid range here, otherwise return an empty
        // string.
        if (begin > end)
            return new StringObject { "", m_encoding };

        // Finally, we can return the substring.
        if (end >= m_length) {
            return new StringObject { m_string.substring(begin), m_encoding };
        } else {
            return new StringObject { m_string.substring(begin, end - begin), m_encoding };
        }
    }

    // First, attempt to convert the index object into an integer, and
    // make sure it fits into a fixnum.
    nat_int_t index = IntegerMethods::convert_to_nat_int_t(env, index_obj);

    // Now, check that the index is within the bounds of the string. If
    // not, then return nil.
    if (index >= m_length || index + m_length < 0)
        return Value::nil();

    // If the index is negative, then we know at this point that it's
    // only negative within one length of the string. So just make it
    // positive here.
    if (index < 0)
        index += m_length;

    // Finally, access the index in the string.
    return new StringObject { m_string.at(index), m_encoding };
}

/**
 * String#bytesplice
 *
 * bytesplice(index, length, str) → string
 * bytesplice(index, length, str, str_index, str_length) → string
 * bytesplice(range, str) → string
 * bytesplice(range, str, str_range) → string
 */
Value StringObject::bytesplice(Env *env, Args &&args) {
    assert_not_frozen(env);

    nat_int_t m_length = static_cast<nat_int_t>(length());
    assert(m_length < NAT_INT_MAX); // not sure how we'd handle a string that big anyway

    auto index_and_length_from_range = [&](String &str, RangeObject *range) {
        auto str_length = static_cast<nat_int_t>(str.length());

        auto index = range->begin().integer();

        // Handle negative start index.
        if (index < -str_length)
            env->raise("RangeError", "{} out of range", range->inspect_str(env));
        if (index < 0)
            index = str_length + index;

        // Handle index past end of string.
        if (index > str_length)
            env->raise("RangeError", "{} out of range", range->inspect_str(env));

        // Handle negative end index.
        auto end = range->end().integer();
        if (end < 0)
            end = str_length + end;

        // Calculate the length.
        Integer length;
        if (range->exclude_end())
            length = end - index;
        else
            length = end - index + 1;

        // Don't let the length be negative.
        if (length < 0)
            length = 0;

        // If the length is longer than the string, truncate it.
        if (length - index > str_length)
            length = str_length - index;

        return std::pair<nat_int_t, nat_int_t> { index.to_nat_int_t(), length.to_nat_int_t() };
    };

    nat_int_t index;
    nat_int_t length;
    StringObject *str = nullptr;
    RangeObject *str_range = nullptr;

    if (args.size() == 2 || (args.size() == 3 && args[0].is_range())) {
        // bytesplice(range, str)
        // bytesplice(range, str, str_range)

        if (!args[0].is_range())
            env->raise("TypeError", "wrong argument type {} (expected Range)", args[0].klass()->inspect_str());

        auto range = args[0].as_range();
        std::tie(index, length) = index_and_length_from_range(m_string, range);

        str = args[1].as_string_or_raise(env);

        if (args.size() == 3) {
            // bytesplice(range, str, str_range)

            auto str_actual_length = static_cast<nat_int_t>(str->length());
            str_range = args[2].as_range_or_raise(env);
            if (str_range->begin().integer() < -str_actual_length)
                env->raise("RangeError", "{} out of range", str_range->inspect_str(env));
        }

    } else if (args.size() == 3 || args.size() == 5) {
        // bytesplice(index, length, str)
        // bytesplice(index, length, str, str_index, str_length)

        index = args[0].integer_or_raise(env).to_nat_int_t();
        if (index < -m_length || index > m_length)
            env->raise("IndexError", "index {} out of string", index);

        if (index < 0)
            index = m_length + index;

        length = args[1].integer_or_raise(env).to_nat_int_t();
        if (length < 0)
            env->raise("IndexError", "negative length {}", length);

        str = args[2].as_string_or_raise(env);

        if (args.size() == 5) {
            // bytesplice(index, length, str, str_index, str_length)

            auto str_actual_length = static_cast<nat_int_t>(str->length());

            auto str_index = args[3].integer_or_raise(env).to_nat_int_t();
            if (str_index < -str_actual_length || str_index > str_actual_length)
                env->raise("IndexError", "index {} out of string", str_index);
            if (str_index < 0)
                str_index = str_actual_length + str_index;

            auto str_length = args[4].integer_or_raise(env).to_nat_int_t();
            if (str_length < 0)
                env->raise("IndexError", "negative length {}", str_length);

            str_range = RangeObject::create(env, Value::integer(str_index), Value::integer(str_index + str_length), true);
        }

    } else {
        env->raise("ArgumentError", "wrong number of arguments (given {}, expected 2, 3, or 5)", args.size());
    }

    auto substr = [&](String &string, EncodingObject *encoding, nat_int_t begin, nat_int_t length) {
        auto string_length = (nat_int_t)string.length();

        if (begin <= string_length) {
            if (!encoding->is_valid_codepoint_boundary(string, begin))
                env->raise("IndexError", "offset {} does not land on character boundary", begin);

            if (length > 0 && !encoding->is_valid_codepoint_boundary(string, begin + length))
                env->raise("IndexError", "offset {} does not land on character boundary", begin + length);
        }

        if (length == 0 || begin >= string_length)
            return String {};

        return string.substring(begin, length);
    };

    auto replacement = str->string();
    if (str_range) {
        auto [str_index, str_length] = index_and_length_from_range(replacement, str_range);
        replacement = substr(replacement, str->encoding(), str_index, str_length);
    }

    if (str) {
        auto compatible_encoding = negotiate_compatible_encoding(str);
        if (!compatible_encoding)
            m_encoding->raise_compatibility_error(env, str->encoding());
        m_encoding = compatible_encoding;
    }

    auto before = substr(m_string, m_encoding.ptr(), 0, index);
    auto after = substr(m_string, m_encoding.ptr(), index + length, m_length - index - length);
    m_string = String::format("{}{}{}", before, replacement, after);

    return this;
}

Value StringObject::slice_in_place(Env *env, Value index_obj, Value length_obj) {
    assert_not_frozen(env);

    // not sure how we'd handle a string that big anyway
    assert(length() < NAT_INT_MAX);

    if (index_obj.is_range()) {
        RangeObject *range = index_obj.as_range();
        auto begin_obj = range->begin();
        auto end_obj = range->end();

        // First, we're going to get the beginning from the range. If it's nil,
        // it's going to be treated as 0. If it's not an integer, it's going to
        // be implicitly converted.
        nat_int_t begin;
        if (begin_obj.is_nil()) {
            begin = 0;
        } else {
            begin = IntegerMethods::convert_to_nat_int_t(env, begin_obj);
        }

        // Shortcut here before doing anything else to return an empty
        // string if the index is right at the end of the string.
        auto count = static_cast<nat_int_t>(char_count(env));
        if (begin == count)
            return new StringObject { "", m_encoding };

        // Now, we're going to do some bounds checks on the beginning of the
        // range. If it's too far outside, we'll return nil.
        if (begin + count < 0 || begin > count)
            return Value::nil();

        // Now, if the beginning is negative, we know it's within one length of
        // the string, so we'll add that here to make it a valid positive index.
        if (begin < 0)
            begin += count;

        // Now we're going to convert the end into a count that we can use.
        nat_int_t end;
        if (end_obj.is_nil()) {
            end = count;
        } else {
            end = IntegerMethods::convert_to_nat_int_t(env, end_obj);
        }

        // Now, we're going to do some bounds checks on the ending of the range.
        // If it's too negative, we'll return an empty string.
        if (end + count < 0)
            return new StringObject { "", m_encoding };

        // If the ending is negative, we know it's within one length of the
        // string, so we'll add that here to make it a valid positive index.
        if (end < 0)
            end += count;

        // After we've done the bounds checks and the negative fixes, we can
        // include the fact that it may be inclusive or not.
        if (!end_obj.is_nil() && !range->exclude_end())
            end++;

        // Finally, we'll delegate over to the ref_slice_range_in_place
        // function.
        return ref_slice_range_in_place(begin, end);
    } else if (index_obj.is_regexp()) {
        // If the index object is a regular expression, then we match against
        // the regular expression and delete the matched string if it exists.
        auto regexp = index_obj.as_regexp();
        auto match_result = regexp->match(env, this);
        int capture = 0;

        // The second argument can be either an index into the match data's
        // captures or the name of a group. If it's not a string, make sure
        // we attempt to convert it into an integer _before_ we return nil
        // if there we no match result.
        if (length_obj != nullptr && !length_obj.is_string())
            capture = IntegerMethods::convert_to_nat_int_t(env, length_obj);

        // If the match failed, return nil. Note that this must happen after
        // the implicit conversion of the index argument to an integer.
        if (match_result.is_nil())
            return match_result;

        // Handle out of bounds checks for the capture index.
        int captures = match_result.as_match_data()->size();
        if (capture + captures <= 0 || capture >= captures)
            return Value::nil();

        // Handle negative capture indices if necessary here.
        if (capture < 0)
            capture += captures;

        // Check if there was a captured segment for the requested capture. If
        // there wasn't return nil.
        ssize_t start_byte_index = match_result.as_match_data()->beg_byte_index(capture);
        ssize_t end_byte_index = match_result.as_match_data()->end_byte_index(capture);
        if (start_byte_index == -1 || end_byte_index == -1)
            return Value::nil();

        // Clone out the matched string for our result first before we mutate
        // the source string.
        ssize_t matched_length = end_byte_index - start_byte_index;
        Value result = new StringObject(&m_string[start_byte_index], static_cast<size_t>(matched_length), m_encoding);

        if (matched_length != 0) {
            // Make sure we back up the match data's source string, since we're
            // going to be modifying the string that its pointing to. This means
            // that $~ should not be impacted by this mutation.
            match_result.as_match_data()->dup_string(env);

            ssize_t byte_length = static_cast<ssize_t>(length());
            memmove(&m_string[start_byte_index], &m_string[end_byte_index], byte_length - end_byte_index);
            m_string.truncate(byte_length - matched_length);
        }

        return result;
    } else if (index_obj.is_string()) {
        // If the index object is a string, then we return the string if it is
        // found as a substring of this string.
        auto start_byte_index = m_string.find(index_obj.as_string()->m_string);

        // If the value wasn't found, we return nil.
        if (start_byte_index == -1)
            return Value::nil();

        // Otherwise, we remove the string and slice it out of the source.
        auto byte_length = length();
        auto end_byte_index = start_byte_index + index_obj.as_string()->length();

        memmove(&m_string[start_byte_index], &m_string[end_byte_index], byte_length - end_byte_index);
        m_string.truncate(byte_length - end_byte_index + start_byte_index);

        return new StringObject { index_obj.as_string()->m_string, index_obj.as_string()->m_encoding };
    } else {
        // First, attempt to convert the index object into an integer, and
        // make sure it fits into a fixnum.
        nat_int_t index = IntegerMethods::convert_to_nat_int_t(env, index_obj);
        nat_int_t count = static_cast<nat_int_t>(char_count(env));

        if (length_obj != nullptr) {
            // First, convert the length to an integer.
            nat_int_t length = IntegerMethods::convert_to_nat_int_t(env, length_obj);

            // Now, check that the index is within the bounds of the string. If
            // not, then return nil.
            if (length < 0 || index >= count || index + count < 0)
                return Value::nil();

            // Shortcut here before doing anything else to return an empty
            // string if the index is right at the end of the string.
            if (index == count || length == 0)
                return new StringObject { "", m_encoding };

            // If the index is negative, then we know at this point that it's
            // only negative within one length of the string. So just make it
            // positive here.
            if (index < 0)
                index += count;

            nat_int_t end = index + length > count ? count : index + length;
            return ref_slice_range_in_place(index, end);
        }

        // Now, check that the index is within the bounds of the string. If
        // not, then return nil.
        if (index >= count || index + count < 0)
            return Value::nil();

        // If the index is negative, then we know at this point that it's
        // only negative within one length of the string. So just make it
        // positive here.
        if (index < 0)
            index += count;

        // We're going to keep track of a moving window starting at the
        // beginning of the string. The prev_byte_index will point to the start
        // of the last character we read, and the next_byte_index will point to
        // the end of the last character we read.
        size_t prev_byte_index = 0;
        size_t next_byte_index = 0;
        size_t char_index = 0;
        size_t byte_length = length();
        StringView c;

        do {
            prev_byte_index = next_byte_index;
            c = next_char(&next_byte_index);
            if (!c.is_empty() && char_index == (size_t)index)
                break;
            char_index++;
        } while (!c.is_empty());

        Value result = new StringObject { c.clone(), m_encoding };
        memmove(&m_string[prev_byte_index], &m_string[prev_byte_index + 1], byte_length - prev_byte_index);
        m_string.truncate(byte_length - 1);
        return result;
    }
}

Value StringObject::ref_slice_range_in_place(size_t begin, size_t end) {
    // We're going to keep track of a moving window starting at the
    // beginning of the string. The prev_byte_index will point to the start
    // of the last character we read, and the next_byte_index will point to
    // the end of the last character we read.
    size_t prev_byte_index = 0;
    size_t next_byte_index = 0;
    size_t start_byte_index = 0;
    size_t char_index = 0;
    size_t byte_length = m_string.length();
    String result;
    StringView c;

    // This loop's responsibility is to advance the prev_byte_index and
    // start_byte_index to the correct point that we can use them to slice
    // and truncate the string.
    do {
        prev_byte_index = next_byte_index;
        c = next_char(&next_byte_index);

        if (!c.is_empty()) {
            if (char_index >= (size_t)end) {
                break;
            } else if (char_index >= (size_t)begin) {
                if (char_index == (size_t)begin)
                    start_byte_index = prev_byte_index;
                result.append(c.clone());
            }
        }
        char_index++;
    } while (!c.is_empty());

    memmove(&m_string[0] + start_byte_index, &m_string[0] + prev_byte_index, byte_length - prev_byte_index);
    m_string.truncate(byte_length - prev_byte_index + start_byte_index);
    return new StringObject { result, m_encoding };
}

Value StringObject::ref_fast_index(Env *env, size_t index) const {
    size_t byte_index = 0;
    size_t char_index = 0;
    StringView c;
    do {
        c = next_char(&byte_index);
        if (!c.is_empty() && char_index == (size_t)index)
            return new StringObject { c, m_encoding };
        char_index++;
    } while (!c.is_empty());
    return Value::nil();
}

Value StringObject::ref_fast_range(Env *env, size_t begin, size_t end) const {
    if (m_encoding->is_single_byte_encoding()) {
        if (begin >= bytesize())
            return Value::nil();
        if (end > bytesize())
            end = bytesize();
        auto length = end - begin;
        return new StringObject { m_string.substring(begin, length), m_encoding };
    }

    size_t byte_index = 0;
    size_t char_index = 0;
    String result;
    StringView c;
    bool char_added = false;
    do {
        c = next_char(&byte_index);
        if (!c.is_empty()) {
            if (char_index >= (size_t)end)
                return new StringObject { result, m_encoding };
            else if (char_index >= (size_t)begin) {
                char_added = true;
                result.append(c.clone());
            }
        }
        char_index++;
    } while (!c.is_empty());
    if (char_index > (size_t)begin || char_added)
        return new StringObject { result, m_encoding };
    return Value::nil();
}

Value StringObject::ref_fast_range_endless(Env *env, size_t begin) const {
    size_t byte_index = 0;
    size_t char_index = 0;
    String result;
    if (char_index < begin) {
        StringView c = next_char(&byte_index);
        char_index++;
        while (!c.is_empty()) {
            if (char_index >= begin)
                break;
            c = next_char(&byte_index);
            char_index++;
        }
    }
    if (char_index < begin)
        return Value::nil();
    for (; byte_index < m_string.length(); byte_index++)
        result.append_char(m_string[byte_index]);
    return new StringObject { result, m_encoding };
}

size_t StringObject::byte_index_to_char_index(ArrayObject *chars, size_t byte_index) {
    size_t char_index = 0;
    size_t current_byte_index = 0;
    for (auto character : *chars) {
        current_byte_index += character.as_string()->length();
        if (current_byte_index > byte_index)
            break;
        ++char_index;
    }
    return char_index;
}

// For String#insert, -1 means *after* the last character.
// For any method looking at the characters, -1 means to include the last character.
ssize_t StringObject::char_index_to_byte_index(ssize_t char_index, bool for_insert) const {
    if (char_index < 0) {
        size_t byte_index = bytesize();
        ssize_t current_char_index = for_insert ? -1 : 0;
        TM::StringView view;
        while (char_index < current_char_index) {
            if (byte_index == 0)
                return -1;
            view = prev_char(&byte_index);
            current_char_index--;
        }
        return byte_index;
    }

    if (m_encoding->is_single_byte_encoding())
        return char_index;

    size_t current_byte_index = 0;
    size_t current_char_index = 0;
    TM::StringView view;
    while (static_cast<size_t>(char_index) > current_char_index) {
        view = next_char(&current_byte_index);
        current_char_index++;
        if (view.is_empty()) return -1;
    }
    return current_byte_index;
}

ssize_t StringObject::byte_index_to_char_index(size_t byte_index) const {
    if (m_encoding->is_single_byte_encoding())
        return byte_index;

    size_t current_byte_index = 0;
    size_t current_char_index = 0;
    TM::StringView view;
    while (byte_index > current_byte_index) {
        view = next_char(&current_byte_index);
        current_char_index++;
        if (view.is_empty()) return -1;
    }
    return current_char_index;
}

Value StringObject::refeq(Env *env, Value arg1, Value arg2, Value value) {
    assert_not_frozen(env);

    if (value == nullptr) {
        value = arg2;
        arg2 = nullptr;
    }

    auto chars = this->chars(env).as_array();
    auto process_begin = [chars, env](nat_int_t begin) -> nat_int_t {
        nat_int_t start = begin;
        if (begin < 0)
            start += chars->size();

        if (start < 0 || start > (nat_int_t)chars->size())
            env->raise("IndexError", "index {} out of string", begin);

        return start;
    };

    auto get_end_by_length = [env](nat_int_t begin, Value length_argument) -> nat_int_t {
        if (length_argument) {
            auto length = IntegerMethods::convert_to_nat_int_t(env, length_argument);
            if (length < 0)
                env->raise("IndexError", "negative length {}", length);
            return begin + length;
        } else {
            return begin + 1;
        }
    };

    nat_int_t begin;
    nat_int_t end = -1;
    nat_int_t expand_length = 0;
    if (arg1.is_integer()) {
        begin = process_begin(arg1.integer().to_nat_int_t());
        end = get_end_by_length(begin, arg2);
    } else if (arg1.is_range()) {
        assert(arg2 == nullptr);
        auto range = arg1.as_range();
        begin = IntegerMethods::convert_to_nat_int_t(env, range->begin());

        // raises a RangeError if Range begin is greater than String size
        if (::abs(begin) >= (nat_int_t)chars->size())
            env->raise("RangeError", "{} out of range", arg1.inspect_str(env));

        // process the begin later to eventually raise the error above
        begin = process_begin(begin);

        end = IntegerMethods::convert_to_nat_int_t(env, range->end());

        // treats a negative out-of-range Range end as a zero count
        if (end < 0 && -end >= (nat_int_t)chars->size()) {
            end = begin;
        } else {
            if (end < 0)
                end += chars->size();

            if (!range->exclude_end())
                ++end;
        }
    } else if (arg1.is_regexp()) {
        auto regexp = arg1.as_regexp();
        auto match_result_value = regexp->match(env, this);
        if (match_result_value.is_nil())
            env->raise("IndexError", "regexp not matched");
        auto match_result = match_result_value.as_match_data();

        nat_int_t match_index_argument = 0;
        if (arg2)
            match_index_argument = IntegerMethods::convert_to_nat_int_t(env, arg2);

        if (::abs(match_index_argument) >= (nat_int_t)match_result->size())
            env->raise("IndexError", "index {} out of regexp", match_index_argument);

        nat_int_t match_index = match_index_argument;
        if (match_index_argument < 0)
            match_index += match_result->size();

        auto offset = match_result->offset(env, Value::integer(match_index)).as_array();
        if (offset->at(0).is_nil())
            env->raise("IndexError", "regexp group {} not matched", match_index);

        begin = IntegerMethods::convert_to_nat_int_t(env, offset->at(0));
        end = IntegerMethods::convert_to_nat_int_t(env, offset->at(1));
    } else if (arg1.is_string()) {
        assert(arg2 == nullptr);
        auto query = arg1.as_string()->string();
        begin = m_string.find(query);
        if (begin == -1)
            env->raise("IndexError", "string not matched");
        begin = byte_index_to_char_index(chars, (size_t)begin);
        end = begin + arg1.as_string()->chars(env).as_array()->size();
    } else {
        begin = process_begin(IntegerMethods::convert_to_nat_int_t(env, arg1));
        end = get_end_by_length(begin, arg2);
    }

    nat_int_t chars_to_be_removed = end - begin;
    if (end > (nat_int_t)chars->size())
        chars_to_be_removed = chars->size() - begin;

    auto string = value.to_str(env);
    auto arg_chars = string->chars(env).as_array();
    size_t new_length = arg_chars->size() + (chars->size() - chars_to_be_removed);

    StringObject result;
    for (size_t i = 0; i < new_length; ++i) {
        if (i < (size_t)begin)
            result.append((*chars)[i]);
        else if (i - begin < arg_chars->size())
            result.append((*arg_chars)[i - begin]);
        else
            result.append((*chars)[i - arg_chars->size() + (end - begin)]);
    }
    m_string = result.string();

    return value;
}

Value StringObject::sub(Env *env, Value find, Value replacement_value, Block *block) {
    if (!block && !replacement_value)
        env->raise("ArgumentError", "wrong number of arguments (given 1, expected 2)");

    if (find.is_string() || find.respond_to(env, "to_str"_s)) {
        const auto pattern = RegexpObject::quote(env, find.to_str(env)).as_string()->string();
        const int options = 0;
        find = new RegexpObject { env, pattern, options };
    }
    if (!find.is_regexp())
        env->raise("TypeError", "wrong argument type {} (expected Regexp)", find.klass()->inspect_str());

    MatchDataObject *match;
    StringObject *expanded_replacement;
    String out;
    regexp_sub(env, out, this, find.as_regexp(), replacement_value, &match, &expanded_replacement, 0, block);

    if (match) {
        // append remaining bytes from source string
        auto byte_index = match->end_byte_index(0);
        if (byte_index >= 0 && (size_t)byte_index < m_string.size())
            out.append(m_string.substring(byte_index));
    }

    return new StringObject { out, m_encoding };
}

Value StringObject::sub_in_place(Env *env, Value find, Value replacement_value, Block *block) {
    assert_not_frozen(env);

    auto replacement = sub(env, find, replacement_value, block).as_string()->string();
    if (m_string == replacement)
        return Value::nil();

    m_string = replacement;
    return this;
}

Value StringObject::gsub(Env *env, Value find, Value replacement_value, Block *block) {
    if (!replacement_value && !block)
        env->raise("NotImplementedError", "Enumerator reply in String#gsub");

    if (find.is_string() || find.respond_to(env, "to_str"_s)) {
        const auto pattern = RegexpObject::quote(env, find.to_str(env)).as_string()->string();
        const int options = 0;
        find = new RegexpObject { env, pattern, options };
    }
    if (!find.is_regexp())
        env->raise("TypeError", "wrong argument type {} (expected Regexp)", find.klass()->inspect_str());

    MatchDataObject *match = nullptr;
    StringObject *expanded_replacement = nullptr;
    size_t byte_index = 0;
    String out;

    do {
        match = nullptr;
        this->regexp_sub(env, out, this, find.as_regexp(), replacement_value, &match, &expanded_replacement, byte_index, block);
        if (match) {
            byte_index = match->end_byte_index(0);
            if (match->is_empty()) {
                // The match was empty, and running it again with the same byte_index would cause an infinite loop.
                // So we increment ahead one byte (and append that one byte), and run the match again.
                if (byte_index < m_string.size())
                    out.append_char(m_string.at(byte_index));
                byte_index++;
            }
        }
    } while (match);

    return new StringObject { out, m_encoding };
}

Value StringObject::gsub_in_place(Env *env, Value find, Value replacement_value, Block *block) {
    assert_not_frozen(env);

    auto replacement = gsub(env, find, replacement_value, block).as_string()->string();
    if (m_string == replacement)
        return Value::nil();

    m_string = replacement;
    return this;
}

Value StringObject::getbyte(Env *env, Value index_obj) const {
    nat_int_t index = IntegerMethods::convert_to_nat_int_t(env, index_obj);
    nat_int_t length = static_cast<nat_int_t>(m_string.length());

    if (index < 0) {
        index += length;
    }

    if (index < 0) {
        return Value::nil();
    }

    if (index >= length) {
        return Value::nil();
    }

    unsigned char byte = m_string[index];
    return Integer(byte);
}

void StringObject::regexp_sub(Env *env, TM::String &out, StringObject *orig_string, RegexpObject *find, Value replacement_value, MatchDataObject **match, StringObject **expanded_replacement, size_t byte_index, Block *block) {
    HashObject *replacement_hash = nullptr;
    StringObject *replacement_str = nullptr;
    if (replacement_value) {
        if (replacement_value.is_hash()) {
            replacement_hash = replacement_value.as_hash();
        } else {
            replacement_str = replacement_value.to_str(env);
        }
        block = nullptr;
    }

    Value match_result = find->match_at_byte_offset(env, orig_string, byte_index);
    if (match_result == Value::nil()) {
        *match = nullptr;
        // append rest of the unmatched bytes to the string
        if (byte_index < orig_string->bytesize())
            out.append(orig_string->string().substring(byte_index));
        return;
    }

    *match = match_result.as_match_data();
    nat_int_t index = (*match)->beg_byte_index(0);

    // unchanged bytes from original string up until match index
    if (byte_index < orig_string->bytesize())
        out.append(orig_string->string().substring(byte_index, index - byte_index));

    if (block) {
        auto string = (*match)->to_s(env);
        Value args[1] = { string };
        Value replacement_from_block = block->run(env, Args(1, args), nullptr);

        *expanded_replacement = replacement_from_block.to_s(env);
        out.append((*expanded_replacement)->string());

        return;
    }

    if (replacement_hash && match) {
        out.append(replacement_hash->ref(env, (*match)->group(0)).to_s(env)->string());
    } else if (replacement_str) {
        *expanded_replacement = expand_backrefs(env, replacement_str, *match);
        out.append((*expanded_replacement)->string());
    }

    return;
}

StringObject *StringObject::expand_backrefs(Env *env, StringObject *str, MatchDataObject *match) {
    const char *c_str = str->c_str();
    size_t len = strlen(c_str);
    StringObject *expanded = new StringObject { "" };
    for (size_t i = 0; i < len; i++) {
        auto c = c_str[i];
        switch (c) {
        case '\\':
            if (i == len - 1) {
                expanded->append_char('\\');
                break;
            }
            c = c_str[++i];
            switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                int num = c - 48;
                auto val = match->group(num);
                if (val.is_string())
                    expanded->append(val.as_string());
                break;
            }
            case '\\':
                expanded->append_char(c);
                break;
            case '&':
                expanded->append(match->group(0));
                break;
            case '`':
                expanded->append(match->pre_match(env));
                break;
            case '\'':
                expanded->append(match->post_match(env));
                break;
            case '+': {
                auto captures = match->captures(env).to_ary(env)->compact(env).to_ary(env);
                if (!captures->is_empty())
                    expanded->append(captures->last());
                break;
            }
            // TODO: there are other back references we need to handle, e.g. \k
            case 'k':
                expanded->append(String::format("<unhandled backref: {}>", c));
                break;
            default:
                expanded->append_char('\\');
                expanded->append_char(c);
                break;
            }
            break;
        default:
            expanded->append_char(c);
        }
    }
    return expanded;
}

Value StringObject::to_c(Env *env) {
    return KernelModule::Complex(env, this, false, true);
}

Value StringObject::to_f(Env *env) const {
    auto result = strtod(c_str(), nullptr);
    return new FloatObject { result };
}

Value StringObject::to_i(Env *env, Value base_obj) const {
    size_t start = 0;

    // strip leading whitespace
    while (start < m_string.size() && isspace(m_string.at(start)))
        start++;

    // skip leading minus sign
    bool negative = false;
    if (start < m_string.size() && m_string.at(start) == '-') {
        negative = true;
        start++;
    }

    // skip leading positive sign
    if (start < m_string.size() && m_string.at(start) == '+' && !negative)
        start++;

    // return 0 for strings with leading underscore
    if (start < m_string.size() && m_string.at(0) == '_')
        return Value::integer(0);

    int base = 10;
    if (base_obj) {
        base = base_obj.to_int(env).to_nat_int_t();

        if (base < 0 || base == 1 || base > 36) {
            env->raise("ArgumentError", "invalid radix {}", base);
        }

        if (base == 0) {
            if (m_string.size() - start >= 2) {
                if (m_string.at(start) == '0') {
                    switch (m_string.at(start + 1)) {
                    case 'b':
                        base = 2;
                        start += 2;
                        break;
                    case 'd':
                        base = 10;
                        start += 2;
                        break;
                    case 'o':
                        base = 8;
                        start += 2;
                        break;
                    case 'x':
                        base = 16;
                        start += 2;
                        break;
                    }
                }
            }
        }
    }

    if (m_string.size() - start >= 2 && m_string.at(start) == '0') {
        auto specifier = m_string.at(start + 1);
        if (specifier == 'b' && base == 2)
            start += 2;
        else if (specifier == 'd' && base == 10)
            start += 2;
        else if (specifier == 'o' && base == 8)
            start += 2;
        else if (specifier == 'x' && base == 16)
            start += 2;
    }

    String digits_only;
    if (negative) digits_only.append_char('-');
    char highest_lower_alpha = 'a' - 1 + (base - 10);
    char highest_upper_alpha = 'A' - 1 + (base - 10);
    for (size_t i = start; i < m_string.size(); ++i) {
        auto c = m_string[i];
        if (isdigit(c)) {
            digits_only.append_char(c);
        } else if (c == '_') {
            // ignore
        } else if (base > 10) {
            if ((c >= 'a' && c <= highest_lower_alpha) || (c >= 'A' && c <= highest_upper_alpha))
                digits_only.append_char(c);
            else
                break;
        } else {
            break;
        }
    }

    nat_int_t number = strtoll(digits_only.c_str(), nullptr, base);
    if (number == NAT_INT_MIN || number == NAT_INT_MAX) {
        return Integer(BigInt(std::move(digits_only), base));
    }
    return Value::integer(number);
}

Value StringObject::to_r(Env *env) const {
    size_t idx = 0;
    String numerator_digits;
    String denominator_digits;
    nat_int_t denominator = 1;

    // ignore leading whitespace
    while (idx < m_string.size() && isspace(m_string.at(idx)))
        idx++;

    // optional hyphen
    if (idx < m_string.size() && m_string.at(idx) == '-') {
        numerator_digits.append_char('-');
        idx++;
    } else if (idx < m_string.size() && m_string.at(idx) == '+') {
        idx++;
    }

    // numerator digits
    for (; idx < m_string.size(); ++idx) {
        auto c = m_string[idx];
        if (isdigit(c)) {
            numerator_digits.append_char(c);
        } else if (c == '_') {
            // ignore underscores between digits
        } else {
            break;
        }
    }

    // optional decimal point and fractional digits
    if (idx < m_string.size() && m_string.at(idx) == '.') {
        idx++;
        while (idx < m_string.size() && isdigit(m_string.at(idx))) {
            numerator_digits.append_char(m_string.at(idx));
            denominator = denominator * 10;
            idx++;
        }
    }

    // optional slash and denominator digits
    if (idx < m_string.size() && m_string.at(idx) == '/') {
        idx++;
        while (idx < m_string.size() && isdigit(m_string.at(idx))) {
            denominator_digits.append_char(m_string.at(idx));
            idx++;
        }
    }

    nat_int_t numerator = strtoll(numerator_digits.c_str(), nullptr, 10);
    if (!denominator_digits.is_empty()) {
        denominator = denominator * strtoll(denominator_digits.c_str(), nullptr, 10);
    }
    return RationalObject::create(env, numerator, denominator);
}

nat_int_t StringObject::unpack_offset(Env *env, Value offset_value) const {
    nat_int_t offset = -1;
    if (offset_value) {
        offset = offset_value.to_int(env).to_nat_int_t();
        if (offset < 0)
            env->raise("ArgumentError", "offset can't be negative");
        else if (offset > (nat_int_t)bytesize())
            env->raise("ArgumentError", "offset outside of string");
    }
    return offset;
}

Value StringObject::unpack(Env *env, Value format, Value offset_value) const {
    auto format_string = format.to_str(env)->string();
    auto offset = unpack_offset(env, offset_value);
    if (offset == (nat_int_t)bytesize())
        return new ArrayObject({ Value::nil() });
    auto unpacker = new StringUnpacker { this, format_string, offset };
    return unpacker->unpack(env);
}

Value StringObject::unpack1(Env *env, Value format, Value offset_value) const {
    auto format_string = format.to_str(env)->string();
    auto offset = unpack_offset(env, offset_value);
    if (offset == (nat_int_t)bytesize())
        return Value::nil();
    auto unpacker = new StringUnpacker { this, format_string, offset };
    return unpacker->unpack1(env);
}

Value StringObject::split(Env *env, RegexpObject *splitter, int max_count) {
    ArrayObject *ary = new ArrayObject {};
    size_t last_index = 0;
    size_t index, len;
    OnigRegion *region = onig_region_new();
    int result = splitter->search(env, this, 0, region, ONIG_OPTION_NONE);
    if (result == ONIG_MISMATCH) {
        ary->push(new StringObject { m_string, m_encoding });
    } else {
        do {
            index = region->beg[0];
            len = region->end[0] - region->beg[0];
            ary->push(new StringObject { &c_str()[last_index], index - last_index, m_encoding });
            last_index = index + len;
            if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                ary->push(new StringObject { &c_str()[last_index], bytesize() - last_index, m_encoding });
                onig_region_free(region, true);
                return ary;
            }
            result = splitter->search(env, this, last_index, region, ONIG_OPTION_NONE);
        } while (result != ONIG_MISMATCH);
        ary->push(new StringObject { &c_str()[last_index], bytesize() - last_index, m_encoding });
    }
    onig_region_free(region, true);
    return ary;
}

Value StringObject::split(Env *env, StringObject *splitstr, int max_count) {
    ArrayObject *ary = new ArrayObject {};
    size_t last_index = 0;
    size_t splitlen = splitstr->length();
    assert(splitlen > 0);
    nat_int_t index = index_int(env, splitstr, 0);
    if (index == -1) {
        ary->push(new StringObject { m_string, m_encoding });
    } else {
        do {
            size_t u_index = static_cast<size_t>(index);
            ary->push(new StringObject { &c_str()[last_index], u_index - last_index, m_encoding });
            last_index = u_index + splitlen;
            if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                ary->push(new StringObject { &c_str()[last_index], bytesize() - last_index, m_encoding });
                return ary;
            }
            index = index_int(env, splitstr, last_index);
        } while (index != -1);
        ary->push(new StringObject { &c_str()[last_index], bytesize() - last_index, m_encoding });
    }
    return ary;
}

// NATFIXME: Does not support:
// + blocks
// + special case single-space splitter
// + proper handling of negative max-count-values

Value StringObject::split(Env *env, Value splitter, Value max_count_value) {
    assert_valid_encoding(env);

    ArrayObject *ary = new ArrayObject {};
    if (!splitter || splitter.is_nil()) {
        auto field_sep = env->global_get("$;"_s);
        if (!field_sep.is_nil()) {
            env->warn("$; is set to non-nil value, but the output was {}", field_sep.klass()->inspect_str());
            splitter = field_sep;
        }
    }
    if (!splitter) {
        splitter = new RegexpObject { env, "\\s+" };
    }
    int max_count = 0;
    if (max_count_value) {
        max_count = IntegerMethods::convert_to_int(env, max_count_value);
    }
    if (length() == 0) {
        return ary;
    } else if (max_count == 1 || splitter.is_nil()) {
        ary->push(new StringObject { m_string, m_encoding });
    } else if (splitter.is_regexp()) {
        // special empty-split-regexp case, just return characters
        if (splitter.as_regexp()->pattern()->is_empty()) {
            ary = this->chars(env).as_array();
        } else {
            // split using regexp
            ary = split(env, splitter.as_regexp(), max_count).as_array();
        }
    } else {
        // string case or object-coercible to string case
        if (!splitter.is_string() && splitter.respond_to(env, "to_str"_s))
            splitter = splitter.to_str(env);
        if (!splitter.is_string())
            env->raise("TypeError", "wrong argument type {} (expected Regexp))", splitter.klass()->inspect_str());

        StringObject *splitstr = splitter.as_string();
        splitstr->assert_valid_encoding(env);

        // special empty-split-string case, just return characters
        if (splitstr->is_empty()) {
            ary = this->chars(env).as_array();
        } else {
            // split using substring
            ary = split(env, splitstr, max_count).as_array();
        }
    }
    if (max_count == 0) {
        // Strip empty trailing strings
        while (!ary->is_empty() && ary->last().as_string()->is_empty())
            ary->pop();
    }
    return ary;
}

bool StringObject::include(Env *env, Value arg) {
    arg = arg.to_str(env);

    auto arg_str = arg.as_string_or_raise(env);

    if (arg_str->is_empty())
        return true;

    assert_compatible_string(env, arg_str);

    return m_string.find(arg.as_string()->m_string) != -1;
}

bool StringObject::include(const char *arg) const {
    return m_string.find(arg) != -1;
}

bool StringObject::include(Env *env, const nat_int_t codepoint) const {
    size_t index = 0;
    for (;;) {
        auto [valid, length, cp] = m_encoding->next_codepoint(m_string, &index);
        if (!valid)
            env->raise_invalid_byte_sequence_error(m_encoding.ptr());
        if (length == 0)
            return false;
        if (codepoint == cp)
            return true;
    }
}

Value StringObject::insert(Env *env, Value index_obj, Value other_str) {
    assert_not_frozen(env);

    auto char_index = IntegerMethods::convert_to_native_type<ssize_t>(env, index_obj.to_int(env));
    StringObject *string = other_str.to_str(env);

    if (char_index == -1) {
        assert_compatible_string_and_update_encoding(env, string);
        append(string);
        return this;
    }

    auto byte_index = char_index_to_byte_index(char_index, true);
    if (byte_index < 0)
        env->raise("IndexError", "index {} out of string", char_index);

    if (string->is_empty())
        return this;

    if (m_encoding != string->m_encoding)
        assert_compatible_string_and_update_encoding(env, string);

    if ((size_t)byte_index == m_string.length()) {
        append(string);
        return this;
    }

    String slice = m_string.substring(byte_index);
    m_string.truncate(byte_index);
    m_string.append(string->m_string);
    m_string.append(slice);

    return this;
}

void StringObject::each_line(Env *env, Value separator, Value chomp_value, std::function<Value(StringObject *)> callback) const {
    if (separator) {
        if (!separator.is_nil())
            separator = separator.to_str(env);
    } else {
        auto dollar_slash = env->global_get("$/"_s);
        if (dollar_slash.is_nil())
            separator = Value::nil();
        else
            separator = dollar_slash.to_str(env);
    }

    auto self_dup = duplicate(env).as_string();

    if (is_empty()) {
        if (separator.is_nil())
            callback(self_dup);
        return;
    }

    if (separator.is_nil())
        separator = new StringObject { "" };

    bool paragraph_mode = false;
    if (separator.as_string()->is_empty()) {
        paragraph_mode = true;
        separator = new StringObject { "\n\n" };
    }

    const auto chomp = chomp_value ? chomp_value.is_truthy() : false;
    auto separator_length = separator.as_string()->length();

    size_t last_index = 0;
    for (;;) {
        auto index = self_dup->index_int(env, separator, last_index);
        auto ptr = &self_dup->c_str()[last_index];

        if (index == -1) {
            auto length = bytesize() - last_index;
            if (length == 0)
                break;
            auto out = new StringObject { ptr, bytesize() - last_index, encoding() };
            callback(out);
            break;
        }

        auto u_index = static_cast<size_t>(index);
        auto length = u_index - last_index + separator_length;

        if (paragraph_mode) {
            for (size_t i = u_index + separator_length; i < bytesize(); i++) {
                if (self_dup->c_str()[i] == '\n')
                    u_index++;
                else
                    break;
            }
        }

        auto out = new StringObject { ptr, length, encoding() };

        if (chomp)
            out->chomp_in_place(env, separator);

        callback(out);

        last_index = u_index + separator_length;
    }
}

Value StringObject::each_line(Env *env, Value separator, Value chomp, Block *block) {
    if (!block) {
        Vector<Value> args { separator };
        auto do_chomp = chomp ? chomp.is_truthy() : false;
        if (do_chomp) {
            auto hash = new HashObject {};
            hash->put(env, "chomp"_s, chomp);
            args.push(hash);
        }
        return enum_for(env, "each_line", Args(args, do_chomp));
    }

    each_line(env, separator, chomp, [&](StringObject *part) -> Value {
        Value args[] = { part };
        block->run(env, Args(1, args), nullptr);
        return this;
    });
    return this;
}

Value StringObject::lines(Env *env, Value separator, Value chomp, Block *block) {
    if (block)
        return each_line(env, separator, chomp, block);

    ArrayObject *ary = new ArrayObject {};
    each_line(env, separator, chomp, [&](StringObject *part) -> Value {
        ary->push(part);
        return this;
    });
    return ary;
}

Value StringObject::ljust(Env *env, Value length_obj, Value pad_obj) const {
    nat_int_t length_i = length_obj.to_int(env).to_nat_int_t();
    size_t length = length_i < 0 ? 0 : length_i;

    StringObject *padstr;
    if (!pad_obj) {
        padstr = new StringObject { " " };
    } else {
        padstr = pad_obj.to_str(env);
    }

    if (padstr->string().is_empty())
        env->raise("ArgumentError", "zero width padding");

    StringObject *copy = new StringObject { m_string, m_encoding };
    while (copy->length() < length) {
        bool truncate = copy->length() + padstr->length() > length;
        copy->append(padstr);
        if (truncate) {
            copy->truncate(length);
        }
    }
    return copy;
}

Value StringObject::strip(Env *env) const {
    if (length() == 0)
        return new StringObject { "", m_encoding };
    assert(length() < NAT_INT_MAX);
    nat_int_t first_char, last_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (first_char = 0; first_char < len; first_char++) {
        char c = c_str()[first_char];
        if (!is_strippable_whitespace(c))
            break;
    }
    for (last_char = len - 1; last_char >= 0; last_char--) {
        char c = c_str()[last_char];
        if (!is_strippable_whitespace(c))
            break;
    }
    if (last_char < 0) {
        return new StringObject { "", m_encoding };
    } else {
        size_t new_length = static_cast<size_t>(last_char - first_char + 1);
        return new StringObject { c_str() + first_char, new_length, m_encoding };
    }
}

Value StringObject::strip_in_place(Env *env) {
    // right side needs to go first because then we have less to move in
    // on the left side
    auto r = rstrip_in_place(env);
    auto l = lstrip_in_place(env);
    return l.is_nil() && r.is_nil() ? Value::nil() : Value(this);
}

Value StringObject::lstrip(Env *env) const {
    if (length() == 0)
        return new StringObject { "", m_encoding };
    assert(length() < NAT_INT_MAX);
    nat_int_t first_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (first_char = 0; first_char < len; first_char++) {
        char c = c_str()[first_char];
        if (!is_strippable_whitespace(c))
            break;
    }
    if (first_char == len) {
        return new StringObject { "", m_encoding };
    } else {
        size_t new_length = static_cast<size_t>(len - first_char);
        return new StringObject { c_str() + first_char, new_length, m_encoding };
    }
}

Value StringObject::lstrip_in_place(Env *env) {
    assert_not_frozen(env);
    if (length() == 0)
        return Value::nil();

    assert(length() < NAT_INT_MAX);
    nat_int_t first_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (first_char = 0; first_char < len; first_char++) {
        char c = c_str()[first_char];
        if (!is_strippable_whitespace(c))
            break;
    }

    if (first_char == 0)
        return Value::nil();

    memmove(&m_string[0], &m_string[0] + first_char, len - first_char);
    m_string.truncate(len - first_char);
    return this;
}

Value StringObject::rjust(Env *env, Value length_obj, Value pad_obj) const {
    nat_int_t length_i = length_obj.to_int(env).to_nat_int_t();
    size_t length = length_i < 0 ? 0 : length_i;

    StringObject *padstr;
    if (!pad_obj) {
        padstr = new StringObject { " " };
    } else {
        padstr = pad_obj.to_str(env);
    }

    if (padstr->string().is_empty())
        env->raise("ArgumentError", "zero width padding");

    StringObject *padding = new StringObject { "", m_encoding };
    size_t padding_length = length < this->length() ? 0 : length - this->length();

    while (padding->length() < padding_length) {
        bool truncate = padding->length() + padstr->length() > padding_length;
        padding->append(padstr);
        if (truncate) {
            padding->truncate(padding_length);
        }
    }

    StringObject *str_with_padding = new StringObject { "", m_encoding };
    StringObject *copy = duplicate(env).as_string();
    str_with_padding->append(padding);
    str_with_padding->append(copy);

    return str_with_padding;
}

Value StringObject::rstrip(Env *env) const {
    if (length() == 0)
        return new StringObject { "", m_encoding };

    if (!valid_encoding()) {
        env->raise(m_encoding->klass()->const_find(env, "CompatibilityError"_s).as_class(), "invalid byte sequence in {}", m_encoding->name()->string());
    }

    assert(length() < NAT_INT_MAX);
    nat_int_t last_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (last_char = len - 1; last_char >= 0; last_char--) {
        char c = c_str()[last_char];
        if (!is_strippable_whitespace(c))
            break;
    }
    if (last_char < 0) {
        return new StringObject { "", m_encoding };
    } else {
        size_t new_length = static_cast<size_t>(last_char + 1);
        return new StringObject { c_str(), new_length, m_encoding };
    }
}

Value StringObject::rstrip_in_place(Env *env) {
    assert_not_frozen(env);
    if (length() == 0)
        return Value::nil();

    if (!valid_encoding())
        env->raise(m_encoding->klass()->const_find(env, "CompatibilityError"_s).as_class(), "invalid byte sequence in {}", m_encoding->name()->string());

    assert(length() < NAT_INT_MAX);
    nat_int_t last_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (last_char = len - 1; last_char >= 0; last_char--) {
        char c = c_str()[last_char];
        if (!is_strippable_whitespace(c))
            break;
    }

    if (last_char == len - 1)
        return Value::nil();

    m_string.truncate(last_char < 0 ? 0 : last_char + 1);
    return this;
}

// This implements checking the case-fold options passed into arguments like
// downcase, upcase, casecmp, etc and sets a bitfield enum.
CaseMapType StringObject::check_case_options(Env *env, Value arg1, Value arg2, bool downcase) {
    SymbolObject *turk = "turkic"_s;
    SymbolObject *lith = "lithuanian"_s;
    // return for zero arg case
    if (arg1.is_null() && arg2.is_null())
        return CaseMapFull;
    // two arg case only accepts turkic and lithuanian (in either order)
    if (!arg1.is_null() && !arg2.is_null()) {
        if ((arg1 == turk && arg2 == lith) || (arg1 == lith && arg2 == turk)) {
            return CaseMapTurkicAzeri | CaseMapLithuanian;
        } else {
            // any other pair of arguments is an error
            env->raise("ArgumentError", "invalid option");
        }
    }
    // acceptable symbols as options: [turkic lithuanian ascii fold]
    if (arg1 == "ascii"_s) {
        return CaseMapAscii;
    } else if (arg1 == "fold"_s) {
        if (downcase) {
            return CaseMapFold;
        } else {
            env->raise("ArgumentError", "option :fold only allowed for downcasing");
        }
    } else if (arg1 == turk) {
        return CaseMapTurkicAzeri;
    } else if (arg1 == lith) {
        return CaseMapLithuanian;
    } else {
        env->raise("ArgumentError", "invalid option");
    }
    return CaseMapFull;
}

// TODO: It is probably more efficient to do the cmp inline so that the
// entire StringObject does not need to be downcased.
Value StringObject::casecmp(Env *env, Value other) {
    other = StringObject::try_convert(env, other);
    if (other.is_nil())
        return Value::nil();

    auto other_str = other.as_string_or_raise(env);

    if (is_empty() && other_str->is_empty())
        return Value::integer(0);

    if (!negotiate_compatible_encoding(other_str))
        return Value::nil();

    auto str1 = this->downcase(env, "ascii"_s, nullptr);
    auto str2 = other_str->downcase(env, "ascii"_s, nullptr);
    return str1->cmp(env, Value(str2));
}

Value StringObject::is_casecmp(Env *env, Value other) {
    other = StringObject::try_convert(env, other);
    if (other.is_nil())
        return Value::nil();

    auto other_str = other.as_string_or_raise(env);

    if (is_empty() && other_str->is_empty())
        return Value::integer(0);

    if (!negotiate_compatible_encoding(other_str))
        return Value::nil();

    auto str1 = this->downcase(env, "fold"_s, nullptr);
    auto str2 = other_str->downcase(env, "fold"_s, nullptr);
    if (str1->string() == str2->string())
        return TrueObject::the();

    return FalseObject::the();
}

StringObject *StringObject::capitalize(Env *env, Value arg1, Value arg2) {
    auto flags = check_case_options(env, arg1, arg2);
    auto str = new StringObject { "", m_encoding };
    bool first_char = true;
    nat_int_t result[3] = {};
    uint8_t length = 0;
    for (StringView c : *this) {
        nat_int_t codepoint = m_encoding->decode_codepoint(c);
        if (first_char)
            length = EncodingObject::codepoint_to_titlecase(codepoint, result, flags);
        else
            length = EncodingObject::codepoint_to_lowercase(codepoint, result, flags);
        for (uint8_t i = 0; i < length; i++)
            str->append(m_encoding->encode_codepoint(result[i]));
        first_char = false;
    }
    return str;
}

Value StringObject::capitalize_in_place(Env *env, Value arg1, Value arg2) {
    auto copy = capitalize(env, arg1, arg2);
    assert_not_frozen(env);
    if (*this == *copy) {
        return Value::nil();
    }
    *this = *copy;
    return this;
}

StringObject *StringObject::downcase(Env *env, Value arg1, Value arg2) {
    auto flags = check_case_options(env, arg1, arg2, true);
    auto str = new StringObject { "", m_encoding };
    nat_int_t result[3] = {};
    for (StringView c : *this) {
        auto codepoint = m_encoding->decode_codepoint(c);
        if (flags & CaseMapAscii) {
            EncodingObject::codepoint_to_lowercase(codepoint, result, flags);
            str->append(m_encoding->encode_codepoint(result[0]));
        } else if ((flags & CaseMapFold || flags & CaseMapLithuanian) && !(flags & CaseMapTurkicAzeri)) {
            auto result = EncodingObject::casefold_full(codepoint);
            if (result.is_array()) {
                for (auto item : *result.as_array()) {
                    str->append(m_encoding->encode_codepoint(item.integer().to_nat_int_t()));
                }
            } else if (result.is_integer()) {
                str->append(m_encoding->encode_codepoint(result.integer().to_nat_int_t()));
            } else {
                str->append(m_encoding->encode_codepoint(codepoint));
            }
        } else {
            auto length = EncodingObject::codepoint_to_lowercase(codepoint, result, flags);
            for (uint8_t i = 0; i < length; i++)
                str->append(m_encoding->encode_codepoint(result[i]));
        }
    }
    return str;
}

Value StringObject::downcase_in_place(Env *env, Value arg1, Value arg2) {
    assert_not_frozen(env);
    StringObject *copy = duplicate(env).as_string();
    *this = *downcase(env, arg1, arg2);

    if (*this == *copy) {
        return Value::nil();
    }
    return this;
}

Value StringObject::dump(Env *env) {
    String out { "\"" };
    auto encoding = m_encoding.ptr();

    auto utf8_encoding = EncodingObject::get(Encoding::UTF_8);

    size_t index = 0;
    auto [valid, ch] = next_char_result(&index);
    while (!ch.is_empty()) {
        if (!valid) {
            for (size_t i = 0; i < ch.size(); i++)
                out.append_sprintf("\\x%02X", static_cast<uint8_t>(ch[i]));
            auto pair = next_char_result(&index);
            valid = pair.first;
            ch = pair.second;
            continue;
        }
        const auto c = encoding->decode_codepoint(ch);
        auto pair = next_char_result(&index);
        valid = pair.first;
        const auto c2 = !valid || ch.is_empty() ? 0 : encoding->decode_codepoint(pair.second);

        if (c == '"' || c == '\\' || (c == '#' && (c2 == '{' || c2 == '$' || c2 == '@'))) {
            out.append_char('\\');
            out.append_char(c);
        } else if (c == '\a') {
            out.append("\\a");
        } else if (c == '\b') {
            out.append("\\b");
        } else if (c == 27) {
            out.append("\\e");
        } else if (c == '\f') {
            out.append("\\f");
        } else if (c == '\n') {
            out.append("\\n");
        } else if (c == '\r') {
            out.append("\\r");
        } else if (c == '\t') {
            out.append("\\t");
        } else if (c == '\v') {
            out.append("\\v");
        } else if (encoding->is_printable_char(c) && c <= 0xFFFF) {
            if (encoding == utf8_encoding || c <= 255)
                out.append(utf8_encoding->encode_codepoint(c));
            else
                encoding->append_escaped_char(out, c);
        } else {
            if (c < 128)
                out.append_sprintf("\\x%02X", c);
            else
                encoding->append_escaped_char(out, c);
        }
        ch = pair.second;
    }

    out.append_char('"');

    if (!m_encoding->is_ascii_compatible())
        out.append_sprintf(".force_encoding(\"%s\")", m_encoding->name()->c_str());

    return new StringObject { std::move(out), m_encoding };
}

StringObject *StringObject::upcase(Env *env, Value arg1, Value arg2) {
    auto flags = check_case_options(env, arg1, arg2);
    auto str = new StringObject { "", m_encoding };
    nat_int_t result[3] = {};
    for (StringView c : *this) {
        auto codepoint = m_encoding->decode_codepoint(c);
        auto length = EncodingObject::codepoint_to_uppercase(codepoint, result, flags);
        for (uint8_t i = 0; i < length; i++)
            str->append(m_encoding->encode_codepoint(result[i]));
    }
    return str;
}

Value StringObject::upcase_in_place(Env *env, Value arg1, Value arg2) {
    assert_not_frozen(env);
    StringObject *copy = duplicate(env).as_string();
    *this = *upcase(env, arg1, arg2);

    if (*this == *copy) {
        return Value::nil();
    }
    return this;
}

StringObject *StringObject::swapcase(Env *env, Value arg1, Value arg2) {
    // currently not doing anything with the returned flags
    check_case_options(env, arg1, arg2);
    auto str = new StringObject { "", m_encoding };
    for (StringView c : *this) {
        nat_int_t codept = m_encoding->decode_codepoint(c);
        if (codept >= 'a' && codept <= 'z') {
            // upcase if codepoint was lowercase
            codept -= 32;
            String s = m_encoding->encode_codepoint(codept);
            str->append(s);
        } else if (codept >= 'A' && codept <= 'Z') {
            // downcase if codepoint was uppercase
            codept += 32;
            String s = m_encoding->encode_codepoint(codept);
            str->append(s);
        } else {
            str->append(c);
        }
    }
    return str;
}

Value StringObject::swapcase_in_place(Env *env, Value arg1, Value arg2) {
    assert_not_frozen(env);
    StringObject *copy = duplicate(env).as_string();
    *this = *swapcase(env, arg1, arg2);
    if (*this == *copy) {
        return Value::nil();
    }
    return this;
}

Value StringObject::uplus(Env *env) {
    if (this->is_frozen()) {
        return this->duplicate(env);
    } else if (is_chilled()) {
        return new StringObject { *this };
    } else {
        return this;
    }
}

Value StringObject::uminus(Env *env) {
    if (this->is_frozen()) {
        return this;
    }
    auto duplicate = this->duplicate(env);
    duplicate->freeze();
    return duplicate;
}

Value StringObject::upto(Env *env, Value other, Value exclusive, Block *block) {
    if (!exclusive)
        exclusive = FalseObject::the();

    if (!block)
        return enum_for(env, "upto", { other, exclusive });

    auto *string = other.to_str(env);

    auto iterator = StringUptoIterator(m_string, string->string(), exclusive.is_truthy());

    if (iterator.treat_as_integer()) {
        if (Integer(string->string()) < Integer(m_string))
            return this;
    } else {
        if (string->length() < length())
            return this;

        if (cmp(env, string).integer() == 1)
            return this;
    }

    TM::Optional<TM::String> current;
    while ((current = iterator.next()).present()) {
        if (current.value().length() > string->length())
            return this;

        block->run(env, { new StringObject(current.value(), m_encoding) }, nullptr);
    }

    return this;
}

Value StringObject::reverse(Env *env) {
    auto str = new StringObject { "", m_encoding };
    if (length() == 0)
        return str;
    auto characters = chars(env).as_array();
    for (size_t i = characters->size() - 1;; i--) {
        str->append((*characters)[i]);
        if (i == 0) break;
    }
    return str;
}

Value StringObject::reverse_in_place(Env *env) {
    this->assert_not_frozen(env);
    *this = *reverse(env).as_string();
    return this;
}

bool StringObject::valid_encoding() const {
    size_t index = 0;
    std::pair<bool, StringView> pair;
    do {
        pair = m_encoding->next_char(m_string, &index);
        if (!pair.first)
            return false;
    } while (!pair.second.is_empty());
    return true;
}

bool StringObject::is_ascii_only() const {
    if (!m_encoding->is_ascii_compatible())
        return false;

    for (size_t i = 0; i < length(); i++) {
        unsigned char c = c_str()[i];
        if (c > 127) {
            return false;
        }
    }
    return true;
}

EncodingObject *StringObject::negotiate_compatible_encoding(const StringObject *other_string) const {
    if (m_encoding == other_string->m_encoding)
        return m_encoding.ptr();

    if (!m_encoding->is_compatible_with(other_string->m_encoding.ptr()))
        return nullptr;

    bool this_is_ascii = is_ascii_only();
    bool other_is_ascii = other_string->is_ascii_only();

    if (!this_is_ascii && !other_is_ascii)
        return nullptr;

    // Special case for BINARY
    if (m_encoding->num() == Encoding::ASCII_8BIT)
        return m_encoding.ptr();

    else if (this_is_ascii && !other_is_ascii)
        return other_string->m_encoding.ptr();
    else
        return m_encoding.ptr();
}

void StringObject::assert_compatible_string(Env *env, const StringObject *other_string) const {
    auto compatible_encoding = negotiate_compatible_encoding(other_string);
    if (compatible_encoding)
        return;

    m_encoding->raise_compatibility_error(env, other_string->m_encoding.ptr());
}

void StringObject::assert_valid_encoding(Env *env) const {
    if (valid_encoding()) return;
    env->raise_invalid_byte_sequence_error(m_encoding.ptr());
}

EncodingObject *StringObject::assert_compatible_string_and_update_encoding(Env *env, StringObject *other_string) {
    auto compatible_encoding = negotiate_compatible_encoding(other_string);
    if (compatible_encoding) {
        if (m_encoding != compatible_encoding)
            m_encoding = compatible_encoding;
        return m_encoding.ptr();
    }

    auto exception_class = fetch_nested_const({ "Encoding"_s, "CompatibilityError"_s }).as_class();
    env->raise(exception_class, "incompatible character encodings: {} and {}", m_encoding->name()->string(), other_string->m_encoding->name()->string());
}

void StringObject::prepend_char(Env *env, char c) {
    m_string.prepend_char(c);
}

void StringObject::append_char(char c) {
    m_string.append_char(c);
}

void StringObject::append(signed char c) {
    m_string.append_char(c);
}

void StringObject::append(unsigned char c) {
    m_string.append_char(c);
}

void StringObject::append(const char *str) {
    m_string.append(str);
}

void StringObject::append(const char *str, size_t bytes) {
    m_string.append(str, bytes);
}

void StringObject::append(int i) {
    m_string.append(i);
}

void StringObject::append(long unsigned int i) {
    m_string.append(i);
}

void StringObject::append(long long int i) {
    m_string.append(i);
}

void StringObject::append(long int i) {
    m_string.append((long long)i);
}

void StringObject::append(unsigned int i) {
    m_string.append((long long)i);
}

void StringObject::append(double d) {
    auto f = FloatObject(d);
    m_string.append(f.to_s());
}

void StringObject::append(const FloatObject *f) {
    m_string.append(f->to_s());
}

void StringObject::append(const String &str) {
    m_string.append(str);
}

void StringObject::append(const StringView &view) {
    m_string += view;
}

void StringObject::append(const StringObject *str) {
    m_string.append(str->string());
}

void StringObject::append(const SymbolObject *sym) {
    m_string.append(sym->string());
}

void StringObject::append(Value val) {
    if (val.is_integer()) {
        append(val.integer().to_string());
        return;
    }

    switch (val.type()) {
    case Type::Array:
    case Type::Hash:
        append(val->dbg_inspect());
        break;
    case Type::False:
        append("false");
        break;
    case Type::Float:
        append(val.is_float());
        break;
    case Type::Nil:
        append("nil");
        break;
    case Type::String:
        append(val.as_string());
        break;
    case Type::Symbol:
        append(val.as_symbol());
        break;
    case Type::True:
        append("true");
        break;
    default:
        append(val->dbg_inspect());
    }
}

Value StringObject::convert_integer(Env *env, nat_int_t base) {
    // Only input bases 0, 2..36 allowed.
    // base default of 0 is for automatic base determination.
    if (base < 0 || base == 1 || base > 36) return nullptr;
    if (m_string.length() < 1) return nullptr;
    // start/end w/ null byte case
    if (m_string[0] == '\0' || m_string.last_char() == '\0') return nullptr;
    if (m_string[0] == '_' || m_string.last_char() == '_') return nullptr;

    auto str = this->strip(env).as_string()->m_string;

    // remove the optional sign character
    auto signchar = str[0];
    auto sign = (signchar == '-') ? -1 : 1;
    if (signchar == '-' || signchar == '+') {
        if (m_string.length() < 2) return nullptr;
        str = str.substring(1);
    }

    // decode the optional base (0x/0o/0d/0b)
    nat_int_t prefix_base = 0;
    if (str[0] == '0' && str.length() > 1) {
        bool next_underscore_valid = false;
        switch (str[1]) {
        case 'x':
        case 'X':
            if (base == 0 || base == 16) {
                prefix_base = 16;
                str = str.substring(2);
            }
            break;
        case 'd':
        case 'D':
            if (base == 0 || base == 10) {
                prefix_base = 10;
                str = str.substring(2);
            }
            break;
        case 'b':
        case 'B':
            if (base == 0 || base == 2) {
                prefix_base = 2;
                str = str.substring(2);
            }
            break;
        case 'o':
        case 'O':
            if (base == 0 || base == 8) {
                prefix_base = 8;
                str = str.substring(2);
            }
            break;
        default:
            if ((base == 0 || base == 8) && isdigit(str[1])) {
                // "implicit" octal with leading zero
                prefix_base = 8;
                str = str.substring(1);
            } else if ((base == 0 || base == 8) && str.length() > 2 && str[1] == '_' && isdigit(str[2])) {
                prefix_base = 8;
                str = str.substring(2);
            }
        }
        // Error if the given input numeric base is not consistent with
        // what is represented in the string.
        if ((base > 0) && (prefix_base > 0) && (base != prefix_base)) {
            return nullptr;
        }
    } else if (str[0] == '+' || str[0] == '-' || str[0] == '_' || is_strippable_whitespace(str[0])) {
        return nullptr;
    } else if (str.length() > 0 && str.last_char() == '_') {
        return nullptr;
    }

    // use the externally given base or the one from the string.
    if (base == 0) {
        base = (prefix_base == 0) ? 10 : prefix_base;
    }
    assert(base >= 2 && base <= 36);

    // invalid for multiple underscores in a row.
    for (size_t i = 0; i < str.length() - 1; ++i) {
        if ((str[i] == '_') && (str[i + 1] == '_')) return nullptr;
    }

    // remove underscores.
    str.remove('_');

    char *end;
    auto convint = strtoll(str.c_str(), &end, base);
    if (convint == NAT_INT_MIN || convint == NAT_INT_MAX) {
        if (signchar == '-') str.prepend_char(signchar);
        return Integer(BigInt(str, base));
    }

    if (end == NULL || end[0] == '\0') {
        return Integer(convint * sign);
    }

    return nullptr;
}
Value StringObject::convert_float() {
    if (m_string.length() == 0 || m_string[0] == '_' || m_string.last_char() == '_') return nullptr;

    auto check_underscores = [this](char delimiter) -> bool {
        ssize_t p = m_string.find(delimiter);
        if (p == -1)
            p = m_string.find((char)toupper(delimiter));

        if (p == -1)
            return true;

        if (p > 0 && m_string[p - 1] == '_')
            return false;

        if (p < (ssize_t)m_string.length() && m_string[p + 1] == '_')
            return false;

        return true;
    };

    if (m_string[0] == '0' && (m_string[1] == 'x' || m_string[1] == 'X') && m_string[2] == '_')
        return nullptr;

    if (!check_underscores('p') || !check_underscores('e')) return nullptr;

    if (m_string.find(0) != -1) return nullptr;

    char *endptr = nullptr;
    String string = String(m_string);

    // check for two consecutive underscores
    for (size_t i = 1; i < string.length(); ++i) {
        auto c2 = string[i];
        auto c1 = string[i - 1];
        if (c1 == '_' && c2 == '_')
            return nullptr;
    }

    string.remove('_');
    string.strip_trailing_whitespace();

    if (string.length() == 0)
        return nullptr;

    double value = strtod(string.c_str(), &endptr);

    if (endptr[0] == '\0') {
        return new FloatObject { value };
    } else {
        return nullptr;
    }
}

Value StringObject::delete_prefix(Env *env, Value val) {
    if (!val.is_string())
        val = val.to_str(env);

    auto arg_len = val.as_string()->length();

    if (start_with(env, { val })) {
        auto after_delete = new StringObject { c_str() + arg_len, length() - arg_len, m_encoding };
        return after_delete;
    }

    StringObject *copy = duplicate(env).as_string();
    return copy;
}

Value StringObject::delete_prefix_in_place(Env *env, Value val) {
    assert_not_frozen(env);
    StringObject *copy = duplicate(env).as_string();
    *this = *delete_prefix(env, val).as_string();

    if (*this == *copy) {
        return Value::nil();
    }
    return this;
}

Value StringObject::delete_suffix(Env *env, Value val) {
    if (!val.is_string())
        val = val.to_str(env);

    auto arg_len = val.as_string()->length();

    if (end_with(env, val)) {
        return new StringObject { c_str(), length() - arg_len, m_encoding };
    }

    return duplicate(env).as_string();
}

Value StringObject::delete_suffix_in_place(Env *env, Value val) {
    assert_not_frozen(env);

    StringObject *copy = duplicate(env).as_string();
    *this = *delete_suffix(env, val).as_string();

    if (*this == *copy) {
        return Value::nil();
    }
    return this;
}

Value StringObject::chop(Env *env) const {
    if (this->is_empty()) {
        return new StringObject { "", m_encoding };
    }

    auto new_str = new StringObject { m_string, m_encoding };
    new_str->chop_in_place(env);
    return new_str;
}

Value StringObject::chop_in_place(Env *env) {
    assert_not_frozen(env);

    if (this->is_empty()) {
        return Value::nil();
    }

    size_t byte_index = length();
    auto removed_char = prev_char(&byte_index);
    auto last_codept = m_encoding->decode_codepoint(removed_char);
    if (last_codept == 0x0A && byte_index > 0) { // LINE_FEED codepoint 0x0A - "\n"
        auto slashr_byte_index = byte_index;
        removed_char = prev_char(&slashr_byte_index);
        last_codept = m_encoding->decode_codepoint(removed_char);
        if (last_codept == 0x0D) // CARRIAGE_RETURN codepoint 0x0D - "\r"
            byte_index = slashr_byte_index;
    }
    m_string.truncate(byte_index);
    return this;
}

Value StringObject::partition(Env *env, Value val) {
    auto ary = new ArrayObject;

    if (val.is_regexp()) {
        auto match_result = val.as_regexp()->match(env, this);

        ssize_t start_byte_index;
        ssize_t end_byte_index;

        if (match_result.is_nil()) {
            return new ArrayObject { new StringObject(*this), new StringObject("", m_encoding), new StringObject("", m_encoding) };
        } else {
            start_byte_index = match_result.as_match_data()->beg_byte_index(0);
            end_byte_index = match_result.as_match_data()->end_byte_index(0);
            ary->push(new StringObject(m_string.substring(0, start_byte_index), m_encoding));
        }

        ary->push(new StringObject(m_string.substring(start_byte_index, end_byte_index - start_byte_index), m_encoding));

        auto last_val_index = start_byte_index + (end_byte_index - start_byte_index);
        auto last_char_index = m_string.length() - 1;
        if (last_val_index < static_cast<long>(last_char_index)) {
            ary->push(new StringObject(m_string.substring(end_byte_index, m_string.length() - end_byte_index), m_encoding));
            return ary;
        }
    } else {
        if (!val.is_string()) {
            val = val.to_str(env);
        }

        auto query = val.as_string()->string();
        auto query_idx = m_string.find(query);

        if (query_idx == -1) {
            return new ArrayObject { new StringObject { m_string, m_encoding }, new StringObject("", m_encoding), new StringObject("", m_encoding) };
        } else {
            ary->push(new StringObject(m_string.substring(0, query_idx), m_encoding));
        }

        ary->push(val);

        auto last_val_index = query_idx + query.length();
        auto last_char_index = m_string.length() - 1;
        if (last_val_index < last_char_index) {
            ary->push(new StringObject(m_string.substring(query_idx + 1, last_char_index - query_idx), m_encoding));
            return ary;
        }
    }

    ary->push(new StringObject("", m_encoding));
    return ary;
}

Value StringObject::sum(Env *env, Value val) {
    int base = 16;
    int sum = 0;

    if (val)
        base = val.to_int(env).to_nat_int_t();

    for (size_t i = 0; i < length(); ++i) {
        sum += m_string[i];
    }

    if (base > 0 && base < 17) {
        int power = ::pow(2, base);
        return Value::integer(sum % power);
    }

    return Value::integer(sum);
}

Value StringObject::try_convert(Env *env, Value val) {
    auto to_str = "to_str"_s;

    if (val.is_string()) {
        return val;
    }

    if (!val.respond_to(env, to_str)) {
        return Value::nil();
    }

    auto result = val.send(env, to_str);

    if (result.is_string() || result.is_nil())
        return result;

    env->raise(
        "TypeError", "can't convert {} to String ({}#to_str gives {})",
        val.klass()->inspect_str(),
        val.klass()->inspect_str(),
        result.klass()->inspect_str());
}
}
