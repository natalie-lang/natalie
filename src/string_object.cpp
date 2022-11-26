#include "ctype.h"
#include "natalie.hpp"
#include "natalie/string_unpacker.hpp"
#include "string.h"

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

std::pair<bool, StringView> StringObject::next_char_result(size_t *index) const {
    return m_encoding->next_char(m_string, index);
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
        Block *size_block = new Block { env, this, StringObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_char"_s }, size_block);
    }

    for (auto c : *this) {
        Value args[] = { new StringObject { c, m_encoding } };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    }
    return this;
}

Value StringObject::chars(Env *env, Block *block) {
    ArrayObject *ary = new ArrayObject {};
    if (block) {
        for (auto c : *this) {
            auto str = new StringObject { c, m_encoding };
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args({ str }), nullptr);
        }
        return this;
    }
    for (auto c : *this)
        ary->push(new StringObject { c, m_encoding });
    return ary;
}

Value StringObject::each_codepoint(Env *env, Block *block) {
    if (!block) {
        Block *size_block = new Block { env, this, StringObject::size_fn, 0 };
        return send(env, "enum_for"_s, { "each_codepoint"_s }, size_block);
    }

    for (auto c : *this) {
        auto char_obj = StringObject { c, m_encoding };

        if (!char_obj.valid_encoding())
            env->raise("ArgumentError", "invalid byte sequence in {}", m_encoding->name()->as_string()->string());

        Value args[] = { char_obj.ord(env) };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    }

    return this;
}

Value StringObject::codepoints(Env *env, Block *block) {
    if (block) {
        for (auto c : *this) {
            auto char_obj = StringObject { c, m_encoding };

            if (!char_obj.valid_encoding())
                env->raise("ArgumentError", "invalid byte sequence in {}", m_encoding->name()->as_string()->string());

            Value args[] = { char_obj.ord(env) };
            NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
        }
        return this;
    }

    if (!this->valid_encoding())
        env->raise("ArgumentError", "invalid byte sequence in {}", m_encoding->name()->as_string()->string());

    ArrayObject *ary = new ArrayObject {};
    for (auto c : *this) {
        auto char_obj = StringObject { c, m_encoding };
        ary->push(char_obj.ord(env));
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
    nat_int_t length_i = length->to_int(env)->to_nat_int_t();

    String pad;

    if (!padstr) {
        pad = String { " " };
    } else if (padstr->is_string()) {
        pad = padstr->as_string()->string();
    } else {
        pad = padstr->to_str(env)->string();
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
    auto new_str = new StringObject { *this };
    new_str->chomp_in_place(env, record_separator);
    return new_str;
}

Value StringObject::chomp_in_place(Env *env, Value record_separator) {
    assert_not_frozen(env);

    // When passed nil, return nil
    if (!record_separator.is_null() && record_separator->is_nil()) {
        return NilObject::the();
    }

    // When passed a non nil object, call to_str();
    if (!record_separator.is_null() && !record_separator->is_string()) {
        record_separator = record_separator->to_str(env);
    }

    if (is_empty()) { // if this is an empty string, return nil
        return NilObject::the();
    }

    size_t end_idx = m_string.length();
    String global_record_separator = env->global_get("$/"_s)->as_string()->string();
    // When using default record separator, also remove trailing \r
    if (record_separator.is_null() && global_record_separator == "\n") {
        if (m_string.at(end_idx - 1) == '\r') {
            --end_idx;
        } else if (m_string.at(end_idx - 1) == '\n') {
            if (m_string.at(end_idx - 2) == '\r') {
                --end_idx;
            }
            --end_idx;
        }

        if (end_idx == m_string.length()) {
            return NilObject::the();
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
            return NilObject::the();
        } else {
            m_string.truncate(end_idx);
            return this;
        }
    }

    record_separator->assert_type(env, Object::Type::String, "String");

    const String rs = record_separator->as_string()->m_string;
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
            return NilObject::the();
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
        return NilObject::the();
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
    return SymbolObject::intern(m_string);
}

Value StringObject::to_sym(Env *env) const {
    return to_symbol(env);
}

Value StringObject::tr(Env *env, Value from_value, Value to_value) const {
    auto copy = dup(env)->as_string();
    copy->tr_in_place(env, from_value, to_value);
    return copy;
}

Value StringObject::tr_in_place(Env *env, Value from_value, Value to_value) {
    assert_not_frozen(env);

    auto from_chars = from_value->to_str(env)->chars(env)->as_array_or_raise(env);
    auto to_chars = to_value->to_str(env)->chars(env)->as_array_or_raise(env);

    // nothing to do
    if (from_chars->is_empty())
        return NilObject::the();

    bool inverted_match = false;
    if (from_chars->size() > 1 && *(from_chars->first()->as_string()) == "^") {
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

            auto c1 = ary->at(i - 2)->as_string();
            auto sep = ary->at(i - 1)->as_string();
            auto c2 = ary->at(i)->as_string();

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
            auto bytes = to_chars->at(replacement_index)->as_string()->string();
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
                if (*from_chars->at(j)->as_string() == c) {
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
                if (*from_chars->at(j)->as_string() == c) {
                    replace_char(byte_index, c.size(), j);
                    break;
                }
            }
            c = prev_char(&byte_index);
        }
    }

    if (!changes_made)
        return NilObject::the();

    return this;
}

StringObject *StringObject::inspect(Env *env) {
    size_t len = length();
    StringObject *out = new StringObject { "\"" };
    for (size_t i = 0; i < len; i++) {
        unsigned char c = m_string[i];
        char c2 = (i + 1) < len ? m_string[i + 1] : 0;
        if (c == '"' || c == '\\' || (c == '#' && c2 == '{')) {
            out->append_char('\\');
            out->append_char(c);
        } else if (c == '\a') {
            out->append("\\a");
        } else if (c == '\b') {
            out->append("\\b");
        } else if ((int)c == 27) { // FIXME: change to '\033' ??
            out->append("\\e");
        } else if (c == '\f') {
            out->append("\\f");
        } else if (c == '\n') {
            out->append("\\n");
        } else if (c == '\r') {
            out->append("\\r");
        } else if (c == '\t') {
            out->append("\\t");
        } else if (c == '\v') {
            out->append("\\v");
        } else if ((int)c < 32 || (int)c == 127 || (int)c == 128) {
            auto escaped_char = m_encoding->escaped_char(c);
            out->append(escaped_char);
        } else {
            out->append_char(c);
        }
    }
    out->append_char('"');
    return out;
}

String StringObject::dbg_inspect() const {
    return String::format("\"{}\"", m_string);
}

StringObject *StringObject::successive(Env *env) {
    auto str = m_string.successive();
    return new StringObject { str, m_encoding };
}

StringObject *StringObject::successive_in_place(Env *env) {
    m_string = m_string.successive();
    return this;
}

bool StringObject::internal_start_with(Env *env, Value needle) const {
    nat_int_t i = index_int(env, needle, 0);
    return i == 0;
}

bool StringObject::start_with(Env *env, Args args) const {
    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = args[i];

        if (internal_start_with(env, arg))
            return true;
    }

    return false;
}

bool StringObject::end_with(Env *env, Value needle) const {
    needle->assert_type(env, Object::Type::String, "String");
    if (length() < needle->as_string()->length())
        return false;
    auto from_end = new StringObject { c_str() + length() - needle->as_string()->length() };
    nat_int_t i = from_end->index_int(env, needle, 0);
    return i == 0;
}

Value StringObject::index(Env *env, Value needle) const {
    return index(env, needle, 0);
}

Value StringObject::index(Env *env, Value needle, size_t start) const {
    auto byte_index = index_int(env, needle, start);
    if (byte_index == -1) {
        return NilObject::the();
    }
    size_t byte_index_size_t = static_cast<size_t>(byte_index);
    size_t char_index = 0, index = 0;
    auto c = next_char(&index);
    while (!c.is_empty()) {
        if (index > byte_index_size_t)
            return IntegerObject::from_size_t(env, char_index);
        char_index++;
        c = next_char(&index);
    }
    return Value::integer(0);
}

nat_int_t StringObject::index_int(Env *env, Value needle, size_t start) const {
    auto needle_str = needle->to_str(env)->as_string()->c_str();

    const char *ptr = strstr(c_str() + start, needle_str);
    if (ptr == nullptr)
        return -1;
    return ptr - c_str();
}

Value StringObject::initialize(Env *env, Value arg) {
    if (arg) {
        arg->assert_type(env, Object::Type::String, "String");
        set_str(arg->as_string()->c_str());
    }
    return this;
}

Value StringObject::initialize_copy(Env *env, Value arg) {
    assert_not_frozen(env);
    if (!arg->is_string() && arg->respond_to(env, "to_str"_s))
        arg = arg->send(env, "to_str"_s);
    arg->assert_type(env, Type::String, "String");
    auto string_obj = arg->as_string();
    m_string = string_obj->string();
    m_encoding = string_obj->encoding();
    return this;
}

Value StringObject::ltlt(Env *env, Value arg) {
    Value args[] = { arg };
    concat(env, Args(1, args));
    return this;
}

Value StringObject::add(Env *env, Value arg) const {
    String str;
    if (arg->is_string()) {
        str = arg->as_string()->string();
    } else {
        str = arg->to_str(env)->string();
    }

    StringObject *new_string = new StringObject { m_string };
    new_string->append(str);
    return new_string;
}

Value StringObject::mul(Env *env, Value arg) const {
    auto nat_int = IntegerObject::convert_to_nat_int_t(env, arg);
    if (nat_int < 0)
        env->raise("ArgumentError", "negative argument");

    if (nat_int && (std::numeric_limits<size_t>::max() / nat_int) < length())
        env->raise("ArgumentError", "argument too big");

    StringObject *new_string = new StringObject { "", m_encoding };
    if (this->is_empty())
        return new_string;

    for (nat_int_t i = 0; i < nat_int; i++) {
        new_string->append(this);
    }
    return new_string;
}

Value StringObject::clear(Env *env) {
    assert_not_frozen(env);

    m_string.clear();

    return this;
}

Value StringObject::cmp(Env *env, Value other) const {
    if (other->type() != Object::Type::String) return NilObject::the();
    auto *str = c_str();
    auto *other_str = other->as_string()->c_str();
    size_t other_length = other->as_string()->length();
    for (size_t i = 0; i < length(); ++i) {
        if (i >= other_length)
            return Value::integer(1);
        if (str[i] < other_str[i])
            return Value::integer(-1);
        else if (str[i] > other_str[i])
            return Value::integer(1);
    }
    if (length() < other_length)
        return Value::integer(-1);
    return Value::integer(0);
}

Value StringObject::concat(Env *env, Args args) {
    assert_not_frozen(env);

    StringObject *original = new StringObject(*this);

    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        StringObject *str_obj;
        if (arg->is_string()) {
            str_obj = arg->as_string();
        } else if (arg->is_integer() && arg->as_integer()->to_nat_int_t() < 0) {
            env->raise("RangeError", "{} out of char range", arg->as_integer()->to_s(env)->as_string()->string());
        } else if (arg->is_integer()) {
            str_obj = arg.send(env, "chr"_s, { m_encoding })->as_string();
        } else {
            str_obj = arg->to_str(env);
        }

        str_obj->assert_type(env, Object::Type::String, "String");

        append(str_obj->string());
    }

    return this;
}

bool StringObject::eq(Env *env, Value arg) {
    if (!arg->is_string() && arg->respond_to(env, "to_str"_s))
        return arg->send(env, "=="_s, { this });
    return eql(arg);
}

Value StringObject::eqtilde(Env *env, Value other) {
    other->assert_type(env, Object::Type::Regexp, "Regexp");
    return other->as_regexp()->eqtilde(env, this);
}

Value StringObject::match(Env *env, Value other) {
    other->assert_type(env, Object::Type::Regexp, "Regexp");
    return other->as_regexp()->match(env, this);
}

Value StringObject::ord(Env *env) const {
    size_t index = 0;
    auto result = next_char_result(&index);
    if (!result.first)
        env->raise("ArgumentError", "invalid byte sequence in {}", m_encoding->name()->as_string()->string());
    auto c = result.second;
    if (c.is_empty())
        env->raise("ArgumentError", "empty string");
    auto code = m_encoding->decode_codepoint(c);
    if (code == -1)
        env->raise("ArgumentError", "invalid byte sequence in {}", m_encoding->name()->as_string()->string());
    return Value::integer(code);
}

Value StringObject::prepend(Env *env, Args args) {
    assert_not_frozen(env);

    StringObject *original = new StringObject(*this);

    String appendable;
    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        StringObject *str_obj;
        if (arg->is_string()) {
            str_obj = arg->as_string();
        } else if (arg->is_integer() && arg->as_integer()->to_nat_int_t() < 0) {
            env->raise("RangeError", "{} out of char range", arg->as_integer()->to_s(env)->as_string()->string());
        } else if (arg->is_integer()) {
            str_obj = arg.send(env, "chr"_s, { m_encoding })->as_string();
        } else {
            str_obj = arg->to_str(env);
        }

        str_obj->assert_type(env, Object::Type::String, "String");
        appendable.append(&str_obj->m_string);
    }
    m_string.prepend(&appendable);

    return this;
}

Value StringObject::b(Env *env) const {
    return new StringObject { m_string, Encoding::ASCII_8BIT };
}

Value StringObject::clone(Env *env) {
    auto duplicate = this->dup(env);
    auto s_class = singleton_class();
    if (s_class) {
        duplicate->set_singleton_class(s_class->clone(env)->as_class());
    }

    if (duplicate->respond_to(env, "initialize_copy"_s)) {
        duplicate->send(env, "initialize_copy"_s, { duplicate });
    }

    if (this->is_frozen()) {
        duplicate->freeze();
    }

    return duplicate;
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
        Block *size_block = new Block { env, this, StringObject::bytesize_fn, 0 };
        return send(env, "enum_for"_s, { "each_byte"_s }, size_block);
    }

    for (size_t i = 0; i < length(); i++) {
        unsigned char c = c_str()[i];
        Value args[] = { Value::integer(c) };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    }
    return this;
}

size_t StringObject::char_count(Env *env) const {
    size_t index = 0;
    size_t char_count = 0;
    auto c = next_char(&index);
    while (!c.is_empty()) {
        char_count++;
        c = next_char(&index);
    }
    return char_count;
}

Value StringObject::setbyte(Env *env, Value index_obj, Value value_obj) {
    assert_not_frozen(env);

    nat_int_t index = IntegerObject::convert_to_nat_int_t(env, index_obj);
    nat_int_t value = IntegerObject::convert_to_nat_int_t(env, value_obj);
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
    return IntegerObject::from_size_t(env, char_count(env));
}

static EncodingObject *find_encoding_by_name(Env *env, String name) {
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

Value StringObject::encode(Env *env, Value encoding) {
    auto orig_encoding = m_encoding;
    StringObject *copy = dup(env)->as_string();
    EncodingObject *encoding_obj;
    switch (encoding->type()) {
    case Object::Type::Encoding:
        encoding_obj = encoding->as_encoding();
        break;
    case Object::Type::String:
        encoding_obj = find_encoding_by_name(env, encoding->as_string()->string());
        break;
    default:
        env->raise("TypeError", "no implicit conversion of {} into String", encoding->klass()->inspect_str());
    }
    return encoding_obj->encode(env, orig_encoding, copy);
}

Value StringObject::force_encoding(Env *env, Value encoding) {
    switch (encoding->type()) {
    case Object::Type::Encoding:
        set_encoding(encoding->as_encoding());
        break;
    case Object::Type::String:
        set_encoding(find_encoding_by_name(env, encoding->as_string()->string()));
        break;
    default:
        env->raise("TypeError", "no implicit conversion of {} into String", encoding->klass()->inspect_str());
    }
    return this;
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
                return new IntegerObject { 0 };
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
                return new IntegerObject { 0 };
            }
            break;
        case prefix:
            if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = char_value;
                state = number;
            } else {
                return new IntegerObject { 0 };
            }
            break;
        case number:
            if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = (value * 16) + char_value;
            } else if (c == '_') {
                state = underscore;
            } else {
                return new IntegerObject { (negative ? -value : value) };
            }
            break;
        case underscore:
            if ((char_value = hex_char_to_decimal_value(c)) != -1) {
                value = (value * 16) + char_value;
                state = number;
            } else {
                return new IntegerObject { (negative ? -value : value) };
            }
            break;
        }
    }

    return new IntegerObject { (negative ? -value : value) };
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
        if (index_obj->is_regexp()) {
            auto regexp = index_obj->as_regexp();
            auto match_result = regexp->match(env, this);

            // The second argument can be either an index into the match data's
            // captures or the name of a group. If it's not a string, make sure
            // we attempt to convert it into an integer _before_ we return nil
            // if there we no match result.
            if (!length_obj->is_string())
                IntegerObject::convert_to_nat_int_t(env, length_obj);

            // If the match failed, return nil. Note that this must happen after
            // the implicit conversion of the index argument to an integer.
            if (match_result->is_nil())
                return match_result;

            // Otherwise, return the region of the string that was captured
            // indicated by the second argument.
            return match_result->as_match_data()->ref(env, length_obj);
        } else {
            // First, attempt to convert the index object into an integer, and
            // make sure it fits into a fixnum.
            nat_int_t index = IntegerObject::convert_to_nat_int_t(env, index_obj);

            // If we have a length, then attempt to convert the length object
            // into an integer, and make sure it fits into a fixnum.
            nat_int_t length = IntegerObject::convert_to_nat_int_t(env, length_obj);
            nat_int_t count = static_cast<nat_int_t>(char_count(env));

            // Now, check that the index is within the bounds of the string. If
            // not, then return nil.
            if (length < 0 || index > count || index + count < 0)
                return NilObject::the();

            // Shortcut here before doing anything else to return an empty
            // string if the index is right at the end of the string.
            if (index == count)
                return new StringObject;

            // If the index is negative, then we know at this point that it's
            // only negative within one length of the string. So just make it
            // positive here.
            if (index < 0)
                index += count;

            nat_int_t end = index + length > count ? count : index + length;
            return ref_fast_range(env, index, end);
        }
    }

    if (index_obj->is_range()) {
        RangeObject *range = index_obj->as_range();
        auto begin_obj = range->begin();
        auto end_obj = range->end();

        // First, we're going to get the beginning from the range. If it's nil,
        // it's going to be treated as 0. If it's not an integer, it's going to
        // be implicitly converted. Finally, we'll assert that it fits into a
        // fixnum.
        nat_int_t begin;
        if (begin_obj->is_nil()) {
            begin = 0;
        } else {
            begin = IntegerObject::convert_to_nat_int_t(env, begin_obj);
        }

        // Shortcut here before doing anything else to return an empty
        // string if the index is right at the end of the string.
        auto count = static_cast<nat_int_t>(char_count(env));
        if (begin == count)
            return new StringObject { "", m_encoding };

        // Now, we're going to do some bounds checks on the beginning of the
        // range. If it's too far outside, we'll return nil.
        if (begin + count < 0 || begin > count)
            return NilObject::the();

        // Now, if the beginning is negative, we know it's within one length of
        // the string, so we'll add that here to make it a valid positive index.
        if (begin < 0)
            begin += count;

        // Now, we're going to shortcut here if the range is endless since we
        // already have all of the information we need.
        if (end_obj->is_nil())
            return ref_fast_range_endless(env, begin);

        // If it's not endless, then we'll go ahead and grab the ending now by
        // converting to an integer and asserting it fits into a fixnum.
        nat_int_t end = IntegerObject::convert_to_nat_int_t(env, end_obj);

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
    } else if (index_obj->is_regexp()) {
        // If the index object is a regular expression, then we'll return the
        // matched substring if there is one. Otherwise, we'll return nil.
        auto regexp = index_obj->as_regexp();
        auto match_result = regexp->match(env, this);

        // If the match failed, return nil.
        if (match_result->is_nil())
            return match_result;

        // Otherwise, return the region of the string that matched.
        return match_result->as_match_data()->to_s(env);
    } else if (index_obj->is_string()) {
        // If the index object is a string, then we return the string if it is
        // found as a substring of this string.
        if (m_string.find(index_obj->as_string()->m_string) != -1)
            return index_obj;

        // Otherwise we return nil.
        return NilObject::the();
    } else {
        // First, attempt to convert the index object into an integer, and
        // make sure it fits into a fixnum.
        nat_int_t index = IntegerObject::convert_to_nat_int_t(env, index_obj);
        nat_int_t count = static_cast<nat_int_t>(char_count(env));

        // Now, check that the index is within the bounds of the string. If
        // not, then return nil.
        if (index > count || index + count < 0)
            return NilObject::the();

        // If the index is negative, then we know at this point that it's
        // only negative within one length of the string. So just make it
        // positive here.
        if (index < 0)
            index += count;

        if (length_obj != nullptr) {
            // If we have a length, then attempt to convert the length object
            // into an integer, and make sure it fits into a fixnum.
            nat_int_t length = IntegerObject::convert_to_nat_int_t(env, length_obj);

            // Now, check that the index is within the bounds of the string. If
            // not, then return nil.
            if (length < 0)
                return NilObject::the();

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
        nat_int_t index = IntegerObject::convert_to_nat_int_t(env, index_obj);
        if (index == m_length)
            return new StringObject("", m_encoding);

        // Next, we're going to get the length that was passed in and make sure
        // that it's not negative. If it is, or the index is too far out of
        // bounds, we'll return nil.
        nat_int_t length = IntegerObject::convert_to_nat_int_t(env, length_obj);
        if (index + m_length < 0 || length < 0 || index > m_length)
            return NilObject::the();

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

    if (index_obj->is_range()) {
        RangeObject *range = index_obj->as_range();
        auto begin_obj = range->begin();
        auto end_obj = range->end();

        // First, we're going to get the beginning from the range. If it's nil,
        // it's going to be treated as 0. If it's not an integer, it's going to
        // be implicitly converted. Finally, we'll assert that it fits into a
        // fixnum.
        nat_int_t begin;
        if (begin_obj->is_nil()) {
            begin = 0;
        } else {
            begin = IntegerObject::convert_to_nat_int_t(env, begin_obj);
        }

        // Shortcut here before doing anything else to return an empty
        // string if the index is right at the end of the string.
        if (begin == m_length)
            return new StringObject { "", m_encoding };

        // Now, we're going to do some bounds checks on the beginning of the
        // range. If it's too far outside, we'll return nil.
        if (begin + m_length < 0 || begin >= m_length)
            return NilObject::the();

        // Now, if the beginning is negative, we know it's within one length of
        // the string, so we'll add that here to make it a valid positive index.
        if (begin < 0)
            begin += m_length;

        // Now, we're going to shortcut here if the range is endless since we
        // already have all of the information we need.
        if (end_obj->is_nil())
            return new StringObject { m_string.substring(begin), m_encoding };

        // If it's not endless, then we'll go ahead and grab the ending now by
        // converting to an integer and asserting it fits into a fixnum.
        nat_int_t end = IntegerObject::convert_to_nat_int_t(env, end_obj);

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
    nat_int_t index = IntegerObject::convert_to_nat_int_t(env, index_obj);

    // Now, check that the index is within the bounds of the string. If
    // not, then return nil.
    if (index >= m_length || index + m_length < 0)
        return NilObject::the();

    // If the index is negative, then we know at this point that it's
    // only negative within one length of the string. So just make it
    // positive here.
    if (index < 0)
        index += m_length;

    // Finally, access the index in the string.
    return new StringObject { m_string.at(index), m_encoding };
}

Value StringObject::slice_in_place(Env *env, Value index_obj, Value length_obj) {
    assert_not_frozen(env);

    // not sure how we'd handle a string that big anyway
    assert(length() < NAT_INT_MAX);

    if (index_obj->is_range()) {
        RangeObject *range = index_obj->as_range();
        auto begin_obj = range->begin();
        auto end_obj = range->end();

        // First, we're going to get the beginning from the range. If it's nil,
        // it's going to be treated as 0. If it's not an integer, it's going to
        // be implicitly converted.
        nat_int_t begin;
        if (begin_obj->is_nil()) {
            begin = 0;
        } else {
            begin = IntegerObject::convert_to_nat_int_t(env, begin_obj);
        }

        // Shortcut here before doing anything else to return an empty
        // string if the index is right at the end of the string.
        auto count = static_cast<nat_int_t>(char_count(env));
        if (begin == count)
            return new StringObject { "", m_encoding };

        // Now, we're going to do some bounds checks on the beginning of the
        // range. If it's too far outside, we'll return nil.
        if (begin + count < 0 || begin > count)
            return NilObject::the();

        // Now, if the beginning is negative, we know it's within one length of
        // the string, so we'll add that here to make it a valid positive index.
        if (begin < 0)
            begin += count;

        // Now we're going to convert the end into a count that we can use.
        nat_int_t end;
        if (end_obj->is_nil()) {
            end = count;
        } else {
            end = IntegerObject::convert_to_nat_int_t(env, end_obj);
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
        if (!end_obj->is_nil() && !range->exclude_end())
            end++;

        // Finally, we'll delegate over to the ref_slice_range_in_place
        // function.
        return ref_slice_range_in_place(begin, end);
    } else if (index_obj->is_regexp()) {
        // If the index object is a regular expression, then we match against
        // the regular expression and delete the matched string if it exists.
        auto regexp = index_obj->as_regexp();
        auto match_result = regexp->match(env, this);
        int capture = 0;

        // The second argument can be either an index into the match data's
        // captures or the name of a group. If it's not a string, make sure
        // we attempt to convert it into an integer _before_ we return nil
        // if there we no match result.
        if (length_obj != nullptr && !length_obj->is_string())
            capture = IntegerObject::convert_to_nat_int_t(env, length_obj);

        // If the match failed, return nil. Note that this must happen after
        // the implicit conversion of the index argument to an integer.
        if (match_result->is_nil())
            return match_result;

        // Handle out of bounds checks for the capture index.
        int captures = match_result->as_match_data()->size();
        if (capture + captures <= 0 || capture >= captures)
            return NilObject::the();

        // Handle negative capture indices if necessary here.
        if (capture < 0)
            capture += captures;

        // Check if there was a captured segment for the requested capture. If
        // there wasn't return nil.
        ssize_t start_byte_index = match_result->as_match_data()->index(capture);
        ssize_t end_byte_index = match_result->as_match_data()->ending(capture);
        if (start_byte_index == -1 || end_byte_index == -1)
            return NilObject::the();

        // Clone out the matched string for our result first before we mutate
        // the source string.
        ssize_t matched_length = end_byte_index - start_byte_index;
        Value result = new StringObject(&m_string[start_byte_index], static_cast<size_t>(matched_length), m_encoding);

        if (matched_length != 0) {
            // Make sure we back up the match data's source string, since we're
            // going to be modifying the string that its pointing to. This means
            // that $~ should not be impacted by this mutation.
            match_result->as_match_data()->dup_string(env);

            ssize_t byte_length = static_cast<ssize_t>(length());
            memmove(&m_string[start_byte_index], &m_string[end_byte_index], byte_length - end_byte_index);
            m_string.truncate(byte_length - matched_length);
        }

        return result;
    } else if (index_obj->is_string()) {
        // If the index object is a string, then we return the string if it is
        // found as a substring of this string.
        auto start_byte_index = m_string.find(index_obj->as_string()->m_string);

        // If the value wasn't found, we return nil.
        if (start_byte_index == -1)
            return NilObject::the();

        // Otherwise, we remove the string and slice it out of the source.
        auto byte_length = length();
        auto end_byte_index = start_byte_index + index_obj->as_string()->length();

        memmove(&m_string[start_byte_index], &m_string[end_byte_index], byte_length - end_byte_index);
        m_string.truncate(byte_length - end_byte_index + start_byte_index);

        return index_obj;
    } else {
        // First, attempt to convert the index object into an integer, and
        // make sure it fits into a fixnum.
        nat_int_t index = IntegerObject::convert_to_nat_int_t(env, index_obj);
        nat_int_t count = static_cast<nat_int_t>(char_count(env));

        if (length_obj != nullptr) {
            // First, convert the length to an integer.
            nat_int_t length = IntegerObject::convert_to_nat_int_t(env, length_obj);

            // Now, check that the index is within the bounds of the string. If
            // not, then return nil.
            if (length < 0 || index >= count || index + count < 0)
                return NilObject::the();

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
            return NilObject::the();

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
    return NilObject::the();
}

Value StringObject::ref_fast_range(Env *env, size_t begin, size_t end) const {
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
    return NilObject::the();
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
        return NilObject::the();
    for (; byte_index < m_string.length(); byte_index++)
        result.append_char(m_string[byte_index]);
    return new StringObject { result, m_encoding };
}

size_t StringObject::byte_index_to_char_index(ArrayObject *chars, size_t byte_index) {
    size_t char_index = 0;
    size_t current_byte_index = 0;
    for (auto character : *chars) {
        current_byte_index += character->as_string()->length();
        if (current_byte_index > byte_index)
            break;
        ++char_index;
    }
    return char_index;
}

Value StringObject::refeq(Env *env, Value arg1, Value arg2, Value value) {
    assert_not_frozen(env);

    if (value == nullptr) {
        value = arg2;
        arg2 = nullptr;
    }

    auto chars = this->chars(env)->as_array();
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
            auto length = IntegerObject::convert_to_nat_int_t(env, length_argument);
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
    if (arg1.is_fast_integer()) {
        begin = process_begin(arg1.get_fast_integer());
        end = get_end_by_length(begin, arg2);
    } else if (arg1->is_range()) {
        assert(arg2 == nullptr);
        auto range = arg1->as_range();
        begin = IntegerObject::convert_to_nat_int_t(env, range->begin());

        // raises a RangeError if Range begin is greater than String size
        if (::abs(begin) >= (nat_int_t)chars->size())
            env->raise("RangeError", "{} out of range", arg1->inspect_str(env));

        // process the begin later to eventually raise the error above
        begin = process_begin(begin);

        end = IntegerObject::convert_to_nat_int_t(env, range->end());

        // treats a negative out-of-range Range end as a zero count
        if (end < 0 && -end >= (nat_int_t)chars->size()) {
            end = begin;
        } else {
            if (end < 0)
                end += chars->size();

            if (!range->exclude_end())
                ++end;
        }
    } else if (arg1->is_regexp()) {
        auto regexp = arg1->as_regexp();
        auto match_result_value = regexp->match(env, this);
        if (match_result_value->is_nil())
            env->raise("IndexError", "regexp not matched");
        auto match_result = match_result_value->as_match_data();

        nat_int_t match_index_argument = 0;
        if (arg2)
            match_index_argument = IntegerObject::convert_to_nat_int_t(env, arg2);

        if (::abs(match_index_argument) >= (nat_int_t)match_result->size())
            env->raise("IndexError", "index {} out of regexp", match_index_argument);

        nat_int_t match_index = match_index_argument;
        if (match_index_argument < 0)
            match_index += match_result->size();

        auto offset = match_result->offset(env, Value::integer(match_index))->as_array();
        if (offset->at(0)->is_nil())
            env->raise("IndexError", "regexp group {} not matched", match_index);

        begin = IntegerObject::convert_to_nat_int_t(env, offset->at(0));
        end = IntegerObject::convert_to_nat_int_t(env, offset->at(1));
    } else if (arg1->is_string()) {
        assert(arg2 == nullptr);
        auto query = arg1->as_string()->string();
        begin = m_string.find(query);
        if (begin == -1)
            env->raise("IndexError", "string not matched");
        begin = byte_index_to_char_index(chars, (size_t)begin);
        end = begin + arg1->as_string()->chars(env)->as_array()->size();
    } else {
        begin = process_begin(IntegerObject::convert_to_nat_int_t(env, arg1));
        end = get_end_by_length(begin, arg2);
    }

    nat_int_t chars_to_be_removed = end - begin;
    if (end > (nat_int_t)chars->size())
        chars_to_be_removed = chars->size();

    auto string = value->to_str(env);
    auto arg_chars = string->chars(env)->as_array();
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
    StringObject *replacement = nullptr;
    if (!block) {
        replacement_value->assert_type(env, Object::Type::String, "String");
        replacement = replacement_value->as_string();
    }
    if (find->is_string()) {
        nat_int_t index = this->index_int(env, find->as_string(), 0);
        if (index == -1) {
            return dup(env)->as_string();
        }
        StringObject *out = new StringObject { c_str(), static_cast<size_t>(index), m_encoding };
        if (block) {
            Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, {}, nullptr);
            result->assert_type(env, Object::Type::String, "String");
            out->append(result->as_string());
        } else {
            out->append(replacement->as_string());
        }
        out->append(&c_str()[index + find->as_string()->length()]);
        return out;
    } else if (find->is_regexp()) {
        MatchDataObject *match;
        StringObject *expanded_replacement;
        return regexp_sub(env, find->as_regexp(), replacement, &match, &expanded_replacement, 0, block);
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp)", find->klass()->inspect_str());
    }
}

Value StringObject::gsub(Env *env, Value find, Value replacement_value, Block *block) {
    StringObject *replacement = nullptr;
    if (replacement_value) {
        replacement_value->assert_type(env, Object::Type::String, "String");
        replacement = replacement_value->as_string();
    }
    if (find->is_string()) {
        NAT_NOT_YET_IMPLEMENTED();
    } else if (find->is_regexp()) {
        MatchDataObject *match = nullptr;
        StringObject *expanded_replacement = nullptr;
        StringObject *result = this;
        size_t start_index = 0;
        do {
            match = nullptr;
            result = result->regexp_sub(env, find->as_regexp(), replacement, &match, &expanded_replacement, start_index, block);
            if (match)
                start_index = match->index(0) + expanded_replacement->length();
        } while (match);
        return result;
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp)", find->klass()->inspect_str());
    }
}

Value StringObject::getbyte(Env *env, Value index_obj) const {
    nat_int_t index = IntegerObject::convert_to_nat_int_t(env, index_obj);
    nat_int_t length = static_cast<nat_int_t>(m_string.length());

    if (index < 0) {
        index += length;
    }

    if (index < 0) {
        return NilObject::the();
    }

    if (index >= length) {
        return NilObject::the();
    }

    unsigned char byte = m_string[index];
    return IntegerObject::create(Integer(byte));
}

StringObject *StringObject::regexp_sub(Env *env, RegexpObject *find, StringObject *replacement, MatchDataObject **match, StringObject **expanded_replacement, size_t start_index, Block *block) {
    Value match_result = find->send(env, "match"_s, { this, Value::integer(start_index) });
    if (match_result == NilObject::the())
        return dup(env)->as_string();
    *match = match_result->as_match_data();
    size_t length = (*match)->as_match_data()->to_s(env)->as_string()->length();
    nat_int_t index = (*match)->as_match_data()->index(0);
    StringObject *out = new StringObject { c_str(), static_cast<size_t>(index), m_encoding };
    if (block) {
        auto string = (*match)->to_s(env);
        Value args[1] = { string };
        Value replacement_from_block = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, Args(1, args), nullptr);
        replacement_from_block->assert_type(env, Object::Type::String, "String");
        *expanded_replacement = replacement_from_block->as_string();
        out->append(*expanded_replacement);
        if (index + length < m_string.length())
            out->append(m_string.substring(index + length));
    } else {
        *expanded_replacement = expand_backrefs(env, replacement->as_string(), *match);
        out->append(*expanded_replacement);
        if (index + length < m_string.length())
            out->append(m_string.substring(index + length));
    }
    return out;
}

StringObject *StringObject::expand_backrefs(Env *env, StringObject *str, MatchDataObject *match) {
    const char *c_str = str->c_str();
    size_t len = strlen(c_str);
    StringObject *expanded = new StringObject { "" };
    for (size_t i = 0; i < len; i++) {
        auto c = c_str[i];
        switch (c) {
        case '\\':
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
                expanded->append(match->group(num)->as_string());
                break;
            }
            case '\\':
                expanded->append_char(c);
                break;
            // TODO: there are other back references we need to handle, e.g. \&, \', \`, and \+
            default:
                fprintf(stderr, "Unknown backslash reference: \\%c", c);
                abort();
            }
            break;
        default:
            expanded->append_char(c);
        }
    }
    return expanded;
}

Value StringObject::to_f(Env *env) const {
    auto result = strtod(c_str(), nullptr);
    return Value::floatingpoint(result);
}

Value StringObject::to_i(Env *env, Value base_obj) const {
    int base = 10;
    if (base_obj) {
        base_obj->assert_type(env, Object::Type::Integer, "Integer");
        base = base_obj->as_integer()->to_nat_int_t();
    }
    nat_int_t number = strtoll(c_str(), nullptr, base);
    return Value::integer(number);
}

Value StringObject::unpack(Env *env, Value format, Value offset_value) const {
    nat_int_t offset = -1;
    if (offset_value) {
        offset = offset_value->to_int(env)->to_nat_int_t();
        if (offset < 0)
            env->raise("ArgumentError", "offset can't be negative");
        else if (offset == (nat_int_t)bytesize())
            return new ArrayObject({ NilObject::the() });
        else if (offset > (nat_int_t)bytesize())
            env->raise("ArgumentError", "offset outside of string");
    }
    auto format_string = format->to_str(env)->string();
    auto unpacker = new StringUnpacker { this, format_string, offset };
    return unpacker->unpack(env);
}

Value StringObject::split(Env *env, Value splitter, Value max_count_value) {
    ArrayObject *ary = new ArrayObject {};
    if (!splitter) {
        splitter = new RegexpObject { env, "\\s+" };
    }
    nat_int_t max_count = 0;
    if (max_count_value) {
        max_count_value->assert_type(env, Object::Type::Integer, "Integer");
        max_count = max_count_value->as_integer()->to_nat_int_t();
    }
    if (length() == 0) {
        return ary;
    } else if (max_count == 1) {
        ary->push(dup(env));
        return ary;
    } else if (splitter->is_regexp()) {
        size_t last_index = 0;
        size_t index, len;
        OnigRegion *region = onig_region_new();
        int result = splitter->as_regexp()->search(c_str(), region, ONIG_OPTION_NONE);
        if (result == ONIG_MISMATCH) {
            ary->push(dup(env));
        } else {
            do {
                index = region->beg[0];
                len = region->end[0] - region->beg[0];
                ary->push(new StringObject { &c_str()[last_index], index - last_index, m_encoding });
                last_index = index + len;
                if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                    ary->push(new StringObject { &c_str()[last_index], m_encoding });
                    onig_region_free(region, true);
                    return ary;
                }
                result = splitter->as_regexp()->search(c_str(), last_index, region, ONIG_OPTION_NONE);
            } while (result != ONIG_MISMATCH);
            ary->push(new StringObject { &c_str()[last_index], m_encoding });
        }
        onig_region_free(region, true);
        return ary;
    } else if (splitter->is_string()) {
        size_t last_index = 0;
        nat_int_t index = index_int(env, splitter->as_string(), 0);
        if (index == -1) {
            ary->push(dup(env));
        } else {
            do {
                size_t u_index = static_cast<size_t>(index);
                ary->push(new StringObject { &c_str()[last_index], u_index - last_index, m_encoding });
                last_index = u_index + splitter->as_string()->length();
                if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                    ary->push(new StringObject { &c_str()[last_index], m_encoding });
                    return ary;
                }
                index = index_int(env, splitter->as_string(), last_index);
            } while (index != -1);
            ary->push(new StringObject { &c_str()[last_index], m_encoding });
        }
        return ary;
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp))", splitter->klass()->inspect_str());
    }
}

bool StringObject::include(Env *env, Value arg) {
    if (!arg->is_string())
        arg = arg->to_str(env);
    return m_string.find(arg->as_string()->m_string) != -1;
}

bool StringObject::include(const char *arg) const {
    return m_string.find(arg) != -1;
}

Value StringObject::ljust(Env *env, Value length_obj, Value pad_obj) const {
    nat_int_t length_i = length_obj->to_int(env)->to_nat_int_t();
    size_t length = length_i < 0 ? 0 : length_i;

    StringObject *padstr;
    if (!pad_obj) {
        padstr = new StringObject { " " };
    } else {
        padstr = pad_obj->to_str(env);
    }

    if (padstr->string().is_empty())
        env->raise("ArgumentError", "zero width padding");

    StringObject *copy = dup(env)->as_string();
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
    // right side needs to go first beacuse then we have less to move in
    // on the left side
    auto r = rstrip_in_place(env);
    auto l = lstrip_in_place(env);
    return l->is_nil() && r->is_nil() ? Value(NilObject::the()) : Value(this);
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
        return NilObject::the();

    assert(length() < NAT_INT_MAX);
    nat_int_t first_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (first_char = 0; first_char < len; first_char++) {
        char c = c_str()[first_char];
        if (!is_strippable_whitespace(c))
            break;
    }

    if (first_char == 0)
        return NilObject::the();

    memmove(&m_string[0], &m_string[0] + first_char, len - first_char);
    m_string.truncate(len - first_char);
    return this;
}

Value StringObject::rjust(Env *env, Value length_obj, Value pad_obj) const {
    nat_int_t length_i = length_obj->to_int(env)->to_nat_int_t();
    size_t length = length_i < 0 ? 0 : length_i;

    StringObject *padstr;
    if (!pad_obj) {
        padstr = new StringObject { " " };
    } else {
        padstr = pad_obj->to_str(env);
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
    StringObject *copy = dup(env)->as_string();
    str_with_padding->append(padding);
    str_with_padding->append(copy);

    return str_with_padding;
}

Value StringObject::rstrip(Env *env) const {
    if (length() == 0)
        return new StringObject { "", m_encoding };
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
        return NilObject::the();

    assert(length() < NAT_INT_MAX);
    nat_int_t last_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (last_char = len - 1; last_char >= 0; last_char--) {
        char c = c_str()[last_char];
        if (!is_strippable_whitespace(c))
            break;
    }

    if (last_char == len - 1)
        return NilObject::the();

    m_string.truncate(last_char < 0 ? 0 : last_char + 1);
    return this;
}

Value StringObject::downcase(Env *env) {
    auto ary = chars(env)->as_array();
    auto str = new StringObject { "", m_encoding };
    for (auto c_val : *ary) {
        auto c_str = c_val->as_string();
        if (c_str->bytesize() > 1) {
            str->append(c_str);
        } else {
            auto c = c_str->c_str()[0];
            if (c >= 65 && c <= 90) {
                c += 32;
            }
            str->append_char(c);
        }
    }
    return str;
}

Value StringObject::upcase(Env *env) {
    auto ary = chars(env)->as_array();
    auto str = new StringObject { "", m_encoding };
    for (auto c_val : *ary) {
        auto c_str = c_val->as_string();
        if (c_str->bytesize() > 1) {
            str->append(c_str);
        } else {
            auto c = c_str->c_str()[0];
            if (c >= 97 && c <= 122) {
                c -= 32;
            }
            str->append_char(c);
        }
    }
    return str;
}

Value StringObject::uplus(Env *env) {
    if (this->is_frozen()) {
        return this->dup(env);
    } else {
        return this;
    }
}

Value StringObject::uminus(Env *env) {
    if (this->is_frozen()) {
        return this;
    }
    auto duplicate = this->dup(env);
    duplicate->freeze();
    return duplicate;
}

Value StringObject::upto(Env *env, Value other, Value exclusive, Block *block) {
    if (!exclusive)
        exclusive = FalseObject::the();

    if (!block)
        return enum_for(env, "upto", { other, exclusive });

    auto *string = other->to_str(env);

    auto iterator = StringUptoIterator(m_string, string->string(), exclusive->is_truthy());

    if (iterator.treat_as_integer()) {
        if (Integer(string->string()) < Integer(m_string))
            return this;
    } else {
        if (string->length() < length())
            return this;

        if (cmp(env, string)->as_integer()->integer() == 1)
            return this;
    }

    TM::Optional<TM::String> current;
    while ((current = iterator.next()).present()) {
        if (current.value().length() > string->length())
            return this;

        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, { new StringObject(current.value(), m_encoding) }, nullptr);
    }

    return this;
}

Value StringObject::reverse(Env *env) {
    if (length() == 0)
        return new StringObject { "", m_encoding };
    auto ary = new ArrayObject { length() };
    auto characters = chars(env)->as_array();
    for (size_t i = characters->size() - 1;; i--) {
        ary->push((*characters)[i]);
        if (i == 0) break;
    }
    return ary->join(env, nullptr);
}

Value StringObject::reverse_in_place(Env *env) {
    this->assert_not_frozen(env);
    *this = *reverse(env)->as_string();
    return this;
}

void StringObject::prepend_char(Env *env, char c) {
    m_string.prepend_char(c);
}

void StringObject::insert(Env *, size_t position, char c) {
    m_string.insert(position, c);
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

void StringObject::append(const IntegerObject *i) {
    m_string.append(i->to_s());
}

void StringObject::append(const String &str) {
    m_string.append(str);
}

void StringObject::append(const StringView &view) {
    m_string.append(view.to_string());
}

void StringObject::append(const StringObject *str) {
    m_string.append(str->string());
}

void StringObject::append(const SymbolObject *sym) {
    m_string.append(sym->string());
}

void StringObject::append(Value val) {
    auto obj = val.object();
    switch (obj->type()) {
    case Type::Array:
    case Type::Hash:
        append(obj->dbg_inspect());
        break;
    case Type::False:
        append("false");
        break;
    case Type::Float:
        append(obj->as_float());
        break;
    case Type::Integer:
        append(obj->as_integer());
        break;
    case Type::Nil:
        append("nil");
        break;
    case Type::String:
        append(obj->as_string());
        break;
    case Type::Symbol:
        append(obj->as_symbol());
        break;
    case Type::True:
        append("true");
        break;
    default:
        append(obj->dbg_inspect());
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

    auto str = this->strip(env)->as_string()->m_string;

    // remove the optional sign character
    auto signchar = str[0];
    auto sign = (signchar == '-') ? -1 : 1;
    if (signchar == '-' || signchar == '+') {
        str = str.substring(1);
    }

    // decode the optional base (0x/0o/0d/0b)
    nat_int_t prefix_base = 0;
    if (str[0] == '0' && str.length() > 1) {
        bool next_underscore_valid = false;
        switch (str[1]) {
        case 'x':
        case 'X':
            prefix_base = 16;
            str = str.substring(2);
            break;
        case 'd':
        case 'D':
            prefix_base = 10;
            str = str.substring(2);
            break;
        case 'b':
        case 'B':
            prefix_base = 2;
            str = str.substring(2);
            break;
        case 'o':
        case 'O':
            prefix_base = 8;
            str = str.substring(2);
            break;
        default:
            if (isdigit(str[1])) {
                // "implicit" octal with leading zero
                prefix_base = 8;
                str = str.substring(1);
            } else if (str.length() > 2 && str[1] == '_' && isdigit(str[2])) {
                prefix_base = 8;
                str = str.substring(2);
            }
        }
        // Error if the given input numeric base is not consistent with
        // what is represented in the string.
        if ((base > 0) && (base != prefix_base)) {
            return nullptr;
        }
    } else if (str[0] == '+' || str[0] == '-' || str[0] == '_' || str[0] == ' ') {
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

    if (end == NULL || end[0] == '\0') {
        return IntegerObject::create(Integer(convint * sign));
    }

    return nullptr;
}
Value StringObject::convert_float() {
    if (m_string[0] == '_' || m_string.last_char() == '_') return nullptr;

    auto check_underscores = [this](char delimiter) -> bool {
        ssize_t p = m_string.find(delimiter);
        if (p == -1)
            p = m_string.find((char)toupper(delimiter));

        if (p != -1 && (m_string[p - 1] == '_' || m_string[p + 1] == '_'))
            return false;

        return true;
    };

    if (m_string[0] == '0' && (m_string[1] == 'x' || m_string[1] == 'X') && m_string[2] == '_')
        return nullptr;

    if (!check_underscores('p') || !check_underscores('e')) return nullptr;

    if (m_string.find(0) != -1) return nullptr;

    char *endptr = nullptr;
    String string = String(m_string);

    string.remove('_');
    string.strip_trailing_whitespace();
    if (string.length() == 0) return nullptr;

    double value = strtod(string.c_str(), &endptr);

    if (endptr[0] == '\0') {
        return new FloatObject { value };
    } else {
        return nullptr;
    }
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

Value StringObject::delete_prefix(Env *env, Value val) {
    if (!val->is_string())
        val = val->to_str(env);

    auto arg_len = val->as_string()->length();
    Value args[] = { val };

    if (start_with(env, Args(1, args))) {
        auto after_delete = new StringObject { c_str() + arg_len, length() - arg_len, m_encoding };
        return after_delete;
    }

    StringObject *copy = dup(env)->as_string();
    return copy;
}

Value StringObject::delete_prefix_in_place(Env *env, Value val) {
    assert_not_frozen(env);
    StringObject *copy = dup(env)->as_string();
    *this = *delete_prefix(env, val)->as_string();

    if (*this == *copy) {
        return Value(NilObject::the());
    }
    return this;
}

Value StringObject::delete_suffix(Env *env, Value val) {
    if (!val->is_string())
        val = val->to_str(env);

    auto arg_len = val->as_string()->length();

    if (end_with(env, val)) {
        return new StringObject { c_str(), length() - arg_len, m_encoding };
    }

    return dup(env)->as_string();
}

Value StringObject::delete_suffix_in_place(Env *env, Value val) {
    assert_not_frozen(env);

    StringObject *copy = dup(env)->as_string();
    *this = *delete_suffix(env, val)->as_string();

    if (*this == *copy) {
        return Value(NilObject::the());
    }
    return this;
}

bool StringObject::ascii_only(Env *env) const {
    for (size_t i = 0; i < length(); i++) {
        unsigned char c = c_str()[i];
        if (c > 127) {
            return false;
        }
    }
    return true;
}

Value StringObject::chop(Env *env) const {
    if (this->is_empty()) {
        return new StringObject { "", m_encoding };
    }

    auto new_str = new StringObject { *this };
    new_str->chop_in_place(env);
    return new_str;
}

Value StringObject::chop_in_place(Env *env) {
    assert_not_frozen(env);

    if (this->is_empty()) {
        return NilObject::the();
    }

    size_t byte_index = length();
    auto removed_char = prev_char(&byte_index);

    if (removed_char == "\n" && byte_index > 0 && m_string[byte_index - 1] == '\r') {
        prev_char(&byte_index);
    }

    m_string.truncate(byte_index);

    return this;
}

Value StringObject::partition(Env *env, Value val) {
    auto ary = new ArrayObject;

    if (val->is_regexp()) {
        auto match_result = val->as_regexp()->match(env, this);

        ssize_t start_byte_index;
        ssize_t end_byte_index;

        if (match_result->is_nil()) {
            return new ArrayObject { new StringObject(*this), new StringObject("", m_encoding), new StringObject("", m_encoding) };
        } else {
            start_byte_index = match_result->as_match_data()->index(0);
            end_byte_index = match_result->as_match_data()->ending(0);
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
        if (!val->is_string()) {
            val = val->to_str(env);
        }

        auto query = val->as_string()->string();
        auto query_idx = m_string.find(query);

        if (query_idx == -1) {
            return new ArrayObject { new StringObject(*this), new StringObject("", m_encoding), new StringObject("", m_encoding) };
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

    if (val) {
        if (!val->is_integer() && val->respond_to(env, "to_int"_s)) {
            val = val->send(env, "to_int"_s);
        }

        val->assert_type(env, Object::Type::Integer, "Integer");

        base = val->as_integer()->to_nat_int_t();
    }

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

    if (val->is_string()) {
        return val;
    }

    if (!val->respond_to(env, to_str)) {
        return NilObject::the();
    }

    auto result = val->send(env, to_str);

    if (result->is_string() || result->is_nil())
        return result;

    env->raise(
        "TypeError", "can't convert {} to String ({}#to_str gives {})",
        val->klass()->inspect_str(),
        val->klass()->inspect_str(),
        result->klass()->inspect_str());
}

}
