#include "ctype.h"
#include "natalie.hpp"
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

StringView StringObject::next_char(size_t *index) const {
    return m_encoding->next_char(m_string, index);
}

Value StringObject::each_char(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "each_char"_s });

    size_t index = 0;
    auto c = next_char(&index);
    while (!c.is_empty()) {
        Value args[] = { new StringObject { c, m_encoding } };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
        c = next_char(&index);
    }
    return NilObject::the();
}

ArrayObject *StringObject::chars(Env *env) {
    ArrayObject *ary = new ArrayObject {};
    for (auto c : *this)
        ary->push(new StringObject { c, m_encoding });
    return ary;
}

String create_padding(String padding, size_t length) {
    size_t quotient = ::floor(length / padding.size());
    size_t remainder = length % padding.size();
    auto buffer = new String { "" };

    for (size_t i = 0; i < quotient; ++i) {
        buffer->append(padding);
    }

    for (size_t j = 0; j < remainder; ++j) {
        buffer->append_char(padding[j]);
    }

    return buffer;
}

Value StringObject::center(Env *env, Value length, Value padstr) {
    nat_int_t length_i = length->to_int(env)->to_nat_int_t();

    auto to_str = "to_str"_s;
    String pad;

    if (!padstr) {
        pad = new String { " " };
    } else if (padstr->is_string()) {
        pad = padstr->as_string()->string();
    } else if (padstr->respond_to(env, to_str)) {
        auto result = padstr->send(env, to_str);

        if (!result->is_string())
            env->raise("TypeError", "padstr can't be converted to a string");

        pad = result->as_string()->string();
    } else {
        env->raise("TypeError", "padstr can't be converted to a string");
    }

    if (pad.is_empty())
        env->raise("ArgumentError", "padstr can't be empty");

    if (length_i <= (nat_int_t)m_string.size())
        return this;

    double split = (length_i - m_string.size()) / 2.0;
    auto left_split = ::floor(split);
    auto right_split = ::ceil(split);

    auto result = new String { m_string };
    result->prepend(create_padding(pad, left_split));
    result->append(create_padding(pad, right_split));

    return new StringObject { result, m_encoding };
}

Value StringObject::chomp(Env *env, Value record_separator) {
    // When passed nil, return duplicate string
    if (!record_separator.is_null() && record_separator->is_nil()) {
        return new StringObject(m_string);
    }

    // When passed a non nil object, call to_str();
    if (!record_separator.is_null() && !record_separator->is_string()) {
        record_separator = record_separator->to_str(env);
    }

    size_t end_idx = m_string.length();
    if (end_idx == 0) { // if this is an empty string, return empty sring
        return new StringObject();
    }

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

        return new StringObject(m_string.substring(0, end_idx));
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

        return new StringObject(m_string.substring(0, end_idx));
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

        return new StringObject(m_string.substring(0, end_idx));
    }

    size_t last_idx = end_idx - 1;
    size_t rs_idx = rs_len - 1;

    // remove only final \r when called with (non empty) string on
    // a string terminating in \r
    if (m_string.at(last_idx) == '\r') {
        return new StringObject(m_string.substring(0, end_idx - 1));
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

    return new StringObject(m_string.substring(0, end_idx));
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
            out->append(env, "\\a");
        } else if (c == '\b') {
            out->append(env, "\\b");
        } else if ((int)c == 27) { // FIXME: change to '\033' ??
            out->append(env, "\\e");
        } else if (c == '\f') {
            out->append(env, "\\f");
        } else if (c == '\n') {
            out->append(env, "\\n");
        } else if (c == '\r') {
            out->append(env, "\\r");
        } else if (c == '\t') {
            out->append(env, "\\t");
        } else if (c == '\v') {
            out->append(env, "\\v");
        } else if ((int)c < 32) {
            auto escaped_char = m_encoding->escaped_char(c);
            out->append(env, escaped_char);
        } else {
            out->append_char(c);
        }
    }
    out->append_char('"');
    return out;
}

StringObject *StringObject::successive(Env *env) {
    auto str = m_string.successive();
    return new StringObject { str };
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
        StringObject *str_obj = arg.send(env, "to_s"_s)->as_string();
        str_obj->assert_type(env, Object::Type::String, "String");
        str = str_obj->as_string()->string();
    }
    StringObject *new_string = new StringObject { m_string };
    new_string->append(env, str);
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
        new_string->append(env, this);
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

    auto to_str = "to_str"_s;
    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        StringObject *str_obj;
        if (arg->is_string()) {
            str_obj = arg->as_string();
        } else if (arg->is_integer() && arg->as_integer()->to_nat_int_t() < 0) {
            env->raise("RangeError", "less than 0");
        } else if (arg->is_integer()) {
            str_obj = arg.send(env, "chr"_s, { m_encoding })->as_string();
        } else if (arg->respond_to(env, to_str)) {
            str_obj = arg.send(env, to_str)->as_string();
        } else {
            env->raise("TypeError", "cannot call to_str", arg->inspect_str(env));
        }

        str_obj->assert_type(env, Object::Type::String, "String");

        append(env, str_obj->string());
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
    auto c = next_char(&index);
    if (c.is_empty())
        env->raise("ArgumentError", "empty string");
    unsigned int code;
    switch (c.size()) {
    case 0:
        NAT_UNREACHABLE();
    case 1:
        code = (unsigned char)c[0];
        break;
    case 2:
        code = (((unsigned char)c[0] ^ 0xC0) << 6) + (((unsigned char)c[1] ^ 0x80) << 0);
        break;
    case 3:
        code = (((unsigned char)c[0] ^ 0xE0) << 12) + (((unsigned char)c[1] ^ 0x80) << 6) + (((unsigned char)c[2] ^ 0x80) << 0);
        break;
    case 4:
        code = (((unsigned char)c[0] ^ 0xF0) << 18) + (((unsigned char)c[1] ^ 0x80) << 12) + (((unsigned char)c[2] ^ 0x80) << 6) + (((unsigned char)c[3] ^ 0x80) << 0);
        break;
    default:
        NAT_UNREACHABLE();
    }
    return Value::integer(code);
}

Value StringObject::prepend(Env *env, Args args) {
    assert_not_frozen(env);

    StringObject *original = new StringObject(*this);

    auto to_str = "to_str"_s;
    String appendable;
    for (size_t i = 0; i < args.size(); i++) {
        auto arg = args[i];

        if (arg == this)
            arg = original;

        StringObject *str_obj;
        if (arg->is_string()) {
            str_obj = arg->as_string();
        } else if (arg->is_integer() && arg->as_integer()->to_nat_int_t() < 0) {
            env->raise("RangeError", "less than 0");
        } else if (arg->is_integer()) {
            str_obj = arg.send(env, "chr"_s, { m_encoding })->as_string();
        } else if (arg->respond_to(env, to_str)) {
            str_obj = arg.send(env, to_str)->as_string();
        } else {
            env->raise("TypeError", "cannot call to_str", arg->inspect_str(env));
        }

        str_obj->assert_type(env, Object::Type::String, "String");
        appendable.append(&str_obj->m_string);
    }
    m_string.prepend(&appendable);

    return this;
}

Value StringObject::b(Env *env) const {
    return new StringObject { m_string.clone(), EncodingObject::get(Encoding::ASCII_8BIT) };
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
    if (!block)
        return send(env, "enum_for"_s, { "each_byte"_s });

    for (size_t i = 0; i < length(); i++) {
        unsigned char c = c_str()[i];
        Value args[] = { Value::integer(c) };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, Args(1, args), nullptr);
    }
    return this;
}

Value StringObject::size(Env *env) const {
    size_t index = 0;
    size_t char_count = 0;
    auto c = next_char(&index);
    while (!c.is_empty()) {
        char_count++;
        c = next_char(&index);
    }
    return IntegerObject::from_size_t(env, char_count);
}

static EncodingObject *find_encoding_by_name(Env *env, const ManagedString *name) {
    ManagedString *lcase_name = name->lowercase();
    ArrayObject *list = EncodingObject::list(env);
    for (size_t i = 0; i < list->size(); i++) {
        EncodingObject *encoding = (*list)[i]->as_encoding();
        ArrayObject *names = encoding->names(env);
        for (size_t n = 0; n < names->size(); n++) {
            StringObject *name_obj = (*names)[n]->as_string();
            ManagedString *name = name_obj->to_low_level_string()->lowercase();
            if (*name == *lcase_name) {
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
        encoding_obj = find_encoding_by_name(env, encoding->as_string()->to_low_level_string());
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
        set_encoding(find_encoding_by_name(env, encoding->as_string()->to_low_level_string()));
        break;
    default:
        env->raise("TypeError", "no implicit conversion of {} into String", encoding->klass()->inspect_str());
    }
    return this;
}

Value StringObject::ref(Env *env, Value index_obj) {
    // not sure how we'd handle a string that big anyway
    assert(length() < NAT_INT_MAX);

    if (index_obj->is_integer()) {
        nat_int_t index = index_obj->as_integer()->to_nat_int_t();

        if (index >= 0)
            return ref_fast_index(env, index);

        ArrayObject *chars = this->chars(env);
        index = chars->size() + index;

        if (index < 0 || index >= (nat_int_t)chars->size())
            return NilObject::the();

        return (*chars)[index];
    } else if (index_obj->is_range()) {
        RangeObject *range = index_obj->as_range();

        auto begin_obj = range->begin();
        nat_int_t begin;
        if (begin_obj->is_nil()) {
            begin = 0;
        } else {
            begin_obj->assert_type(env, Object::Type::Integer, "Integer");
            begin = begin_obj->as_integer()->to_nat_int_t();
        }

        auto end_obj = range->end();

        if (begin >= 0 && end_obj->is_integer() && end_obj->as_integer()->to_nat_int_t() > 0) {
            auto end = end_obj->as_integer()->to_nat_int_t();
            if (!range->exclude_end())
                end++;
            return ref_fast_range(env, begin, end);
        } else if (end_obj->is_nil()) {
            return ref_fast_range_endless(env, begin);
        }

        ArrayObject *chars = this->chars(env);

        nat_int_t end;
        if (end_obj->is_nil()) {
            end = range->exclude_end() ? chars->size() + 1 : chars->size();
        } else {
            end_obj->assert_type(env, Object::Type::Integer, "Integer");
            end = end_obj->as_integer()->to_nat_int_t();
        }

        if (begin < 0) begin = chars->size() + begin;
        if (end < 0) end = chars->size() + end;

        if ((begin < 0 || begin > (nat_int_t)chars->size()) && (end < 0 || end >= (nat_int_t)chars->size()))
            return NilObject::the();

        if (begin < 0 || end < 0)
            return new StringObject;

        size_t u_begin = static_cast<size_t>(begin);
        size_t u_end = static_cast<size_t>(end);
        if (u_begin > chars->size()) return NilObject::the();

        if (!range->exclude_end()) u_end++;

        size_t max = chars->size();
        u_end = u_end > max ? max : u_end;
        StringObject *result = new StringObject {};
        for (size_t i = begin; i < u_end; i++) {
            result->append(env, (*chars)[i]);
        }

        return result;
    }
    index_obj->assert_type(env, Object::Type::Integer, "Integer");
    abort();
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
                return new StringObject { result };
            else if (char_index >= (size_t)begin) {
                char_added = true;
                result.append(c.clone());
            }
        }
        char_index++;
    } while (!c.is_empty());
    if (char_index > (size_t)begin || char_added)
        return new StringObject { result };
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
    return new StringObject { result };
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

    auto chars = this->chars(env);
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
        end = begin + arg1->as_string()->chars(env)->size();
    } else {
        begin = process_begin(IntegerObject::convert_to_nat_int_t(env, arg1));
        end = get_end_by_length(begin, arg2);
    }

    nat_int_t chars_to_be_removed = end - begin;
    if (end > (nat_int_t)chars->size())
        chars_to_be_removed = chars->size();

    auto string = value->to_str(env);
    auto arg_chars = string->chars(env);
    size_t new_length = arg_chars->size() + (chars->size() - chars_to_be_removed);

    StringObject result;
    for (size_t i = 0; i < new_length; ++i) {
        if (i < (size_t)begin)
            result.append(env, (*chars)[i]);
        else if (i - begin < arg_chars->size())
            result.append(env, (*arg_chars)[i - begin]);
        else
            result.append(env, (*chars)[i - arg_chars->size() + (end - begin)]);
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
        StringObject *out = new StringObject { c_str(), static_cast<size_t>(index) };
        if (block) {
            Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, {}, nullptr);
            result->assert_type(env, Object::Type::String, "String");
            out->append(env, result->as_string());
        } else {
            out->append(env, replacement->as_string());
        }
        out->append(env, &c_str()[index + find->as_string()->length()]);
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

StringObject *StringObject::regexp_sub(Env *env, RegexpObject *find, StringObject *replacement, MatchDataObject **match, StringObject **expanded_replacement, size_t start_index, Block *block) {
    Value match_result = find->send(env, "match"_s, { this, Value::integer(start_index) });
    if (match_result == NilObject::the())
        return dup(env)->as_string();
    *match = match_result->as_match_data();
    size_t length = (*match)->as_match_data()->group(env, 0)->as_string()->length();
    nat_int_t index = (*match)->as_match_data()->index(0);
    StringObject *out = new StringObject { c_str(), static_cast<size_t>(index) };
    if (block) {
        auto string = (*match)->to_s(env);
        Value args[1] = { string };
        Value replacement_from_block = NAT_RUN_BLOCK_WITHOUT_BREAK(env, block, Args(1, args), nullptr);
        replacement_from_block->assert_type(env, Object::Type::String, "String");
        *expanded_replacement = replacement_from_block->as_string();
        out->append(env, *expanded_replacement);
        if (index + length < m_string.length())
            out->append(env, m_string.substring(index + length));
    } else {
        *expanded_replacement = expand_backrefs(env, replacement->as_string(), *match);
        out->append(env, *expanded_replacement);
        if (index + length < m_string.length())
            out->append(env, m_string.substring(index + length));
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
                expanded->append(env, match->group(env, num)->as_string());
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

Value StringObject::to_i(Env *env, Value base_obj) const {
    int base = 10;
    if (base_obj) {
        base_obj->assert_type(env, Object::Type::Integer, "Integer");
        base = base_obj->as_integer()->to_nat_int_t();
    }
    nat_int_t number = strtoll(c_str(), nullptr, base);
    return Value::integer(number);
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
                ary->push(new StringObject { &c_str()[last_index], index - last_index });
                last_index = index + len;
                if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                    ary->push(new StringObject { &c_str()[last_index] });
                    onig_region_free(region, true);
                    return ary;
                }
                result = splitter->as_regexp()->search(c_str(), last_index, region, ONIG_OPTION_NONE);
            } while (result != ONIG_MISMATCH);
            ary->push(new StringObject { &c_str()[last_index] });
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
                ary->push(new StringObject { &c_str()[last_index], u_index - last_index });
                last_index = u_index + splitter->as_string()->length();
                if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                    ary->push(new StringObject { &c_str()[last_index] });
                    return ary;
                }
                index = index_int(env, splitter->as_string(), last_index);
            } while (index != -1);
            ary->push(new StringObject { &c_str()[last_index] });
        }
        return ary;
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp))", splitter->klass()->inspect_str());
    }
}

bool StringObject::include(Env *env, Value arg) {
    auto to_str = "to_str"_s;
    if (!arg->is_string() && arg->respond_to(env, to_str))
        arg = arg->send(env, to_str);
    arg->assert_type(env, Object::Type::String, "String");
    return m_string.find(arg->as_string()->m_string) != -1;
}

bool StringObject::include(const char *arg) const {
    return m_string.find(arg) != -1;
}

Value StringObject::ljust(Env *env, Value length_obj, Value pad_obj) {
    nat_int_t length_i = length_obj->to_int(env)->to_nat_int_t();
    size_t length = length_i < 0 ? 0 : length_i;

    StringObject *padstr;
    if (!pad_obj) {
        padstr = new StringObject { " " };
    } else {
        padstr = pad_obj->to_str(env);
    }

    if (padstr->string().is_empty())
        env->raise("ArgumentError", "can't pad with an empty string");

    StringObject *copy = dup(env)->as_string();
    while (copy->length() < length) {
        bool truncate = copy->length() + padstr->length() > length;
        copy->append(env, padstr);
        if (truncate) {
            copy->truncate(length);
        }
    }
    return copy;
}

Value StringObject::strip(Env *env) const {
    if (length() == 0)
        return new StringObject {};
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
        return new StringObject {};
    } else {
        size_t new_length = static_cast<size_t>(last_char - first_char + 1);
        return new StringObject { c_str() + first_char, new_length };
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
        return new StringObject {};
    assert(length() < NAT_INT_MAX);
    nat_int_t first_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (first_char = 0; first_char < len; first_char++) {
        char c = c_str()[first_char];
        if (!is_strippable_whitespace(c))
            break;
    }
    if (first_char == len) {
        return new StringObject {};
    } else {
        size_t new_length = static_cast<size_t>(len - first_char);
        return new StringObject { c_str() + first_char, new_length };
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

Value StringObject::rstrip(Env *env) const {
    if (length() == 0)
        return new StringObject {};
    assert(length() < NAT_INT_MAX);
    nat_int_t last_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (last_char = len - 1; last_char >= 0; last_char--) {
        char c = c_str()[last_char];
        if (!is_strippable_whitespace(c))
            break;
    }
    if (last_char < 0) {
        return new StringObject {};
    } else {
        size_t new_length = static_cast<size_t>(last_char + 1);
        return new StringObject { c_str(), new_length };
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
    auto ary = chars(env);
    auto str = new StringObject {};
    for (auto c_val : *ary) {
        auto c_str = c_val->as_string();
        if (c_str->bytesize() > 1) {
            str->append(env, c_str);
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
    auto ary = chars(env);
    auto str = new StringObject {};
    for (auto c_val : *ary) {
        auto c_str = c_val->as_string();
        if (c_str->bytesize() > 1) {
            str->append(env, c_str);
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

Value StringObject::reverse(Env *env) {
    if (length() == 0)
        return new StringObject {};
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

void StringObject::append(Env *, const char *str) {
    m_string.append(str);
}

void StringObject::append(Env *, const StringObject *str) {
    m_string.append(str->string());
}

void StringObject::append(Env *, const ManagedString *str) {
    m_string.append(str);
}

void StringObject::append(Env *, const String &str) {
    m_string.append(str);
}

void StringObject::append(Env *env, Value val) {
    if (val->is_string())
        append(env, val->as_string()->c_str());
    else
        append(env, val.send(env, "to_s"_s));
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
    try {
        StringView c = next_char(&index);
        while (!c.is_empty()) {
            c = next_char(&index);
        }
    } catch (ExceptionObject *) {
        return false;
    }
    return true;
}

Value StringObject::delete_prefix(Env *env, Value val) {
    auto to_str = "to_str"_s;
    if (!val->is_string() && val->respond_to(env, to_str))
        val = val->send(env, to_str);
    val->assert_type(env, Object::Type::String, "String");

    auto arg_len = val->as_string()->length();
    Value args[] = { val };

    if (start_with(env, Args(1, args))) {
        auto after_delete = new StringObject { c_str() + arg_len, length() - arg_len };
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
    auto to_str = "to_str"_s;
    if (!val->is_string() && val->respond_to(env, to_str))
        val = val->send(env, to_str);
    val->assert_type(env, Object::Type::String, "String");

    auto arg_len = val->as_string()->length();

    if (end_with(env, val)) {
        return new StringObject { c_str(), length() - arg_len };
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

}
