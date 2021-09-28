#include "ctype.h"
#include "natalie.hpp"
#include "string.h"

namespace Natalie {

void StringValue::raise_encoding_invalid_byte_sequence_error(Env *env, size_t index) {
    StringValue *message = format(env, "invalid byte sequence at index {} in string of size {} (string not long enough)", index, length());
    ClassValue *Encoding = GlobalEnv::the()->Object()->const_find(env, SymbolValue::intern("Encoding"))->as_class();
    ClassValue *InvalidByteSequenceError = Encoding->const_find(env, SymbolValue::intern("InvalidByteSequenceError"))->as_class();
    ExceptionValue *exception = new ExceptionValue { InvalidByteSequenceError, message };
    env->raise_exception(exception);
}

char *StringValue::next_char(Env *env, char *buffer, size_t *index) {
    size_t len = length();
    if (*index >= len)
        return nullptr;
    size_t i = *index;
    const char *str = m_string.c_str();
    switch (m_encoding) {
    case Encoding::UTF_8:
        buffer[0] = str[i];
        if (((unsigned char)buffer[0] >> 3) == 30) { // 11110xxx, 4 bytes
            if (i + 3 >= len) raise_encoding_invalid_byte_sequence_error(env, i);
            buffer[1] = str[++i];
            buffer[2] = str[++i];
            buffer[3] = str[++i];
            buffer[4] = 0;
        } else if (((unsigned char)buffer[0] >> 4) == 14) { // 1110xxxx, 3 bytes
            if (i + 2 >= len) raise_encoding_invalid_byte_sequence_error(env, i);
            buffer[1] = str[++i];
            buffer[2] = str[++i];
            buffer[3] = 0;
        } else if (((unsigned char)buffer[0] >> 5) == 6) { // 110xxxxx, 2 bytes
            if (i + 1 >= len) raise_encoding_invalid_byte_sequence_error(env, i);
            buffer[1] = str[++i];
            buffer[2] = 0;
        } else {
            buffer[1] = 0;
        }
        *index = i + 1;
        return buffer;
    case Encoding::ASCII_8BIT:
        buffer[0] = str[i];
        buffer[1] = 0;
        (*index)++;
        return buffer;
    }
    NAT_UNREACHABLE();
}

ValuePtr StringValue::each_char(Env *env, Block *block) {
    if (!block)
        return send(env, SymbolValue::intern("enum_for"), { SymbolValue::intern("each_char") });

    size_t index = 0;
    char buffer[5];
    while (next_char(env, buffer, &index)) {
        auto c = new StringValue { buffer, m_encoding };
        ValuePtr args[] = { c };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
    }
    return NilValue::the();
}

ArrayValue *StringValue::chars(Env *env) {
    ArrayValue *ary = new ArrayValue {};
    size_t index = 0;
    char buffer[5];
    while (next_char(env, buffer, &index)) {
        auto c = new StringValue { buffer, m_encoding };
        ary->push(c);
    }
    return ary;
}

SymbolValue *StringValue::to_symbol(Env *env) {
    return SymbolValue::intern(c_str());
}

ValuePtr StringValue::to_sym(Env *env) {
    return to_symbol(env);
}

StringValue *StringValue::inspect(Env *env) {
    size_t len = length();
    const char *str = m_string.c_str();
    StringValue *out = new StringValue { "\"" };
    for (size_t i = 0; i < len; i++) {
        unsigned char c = str[i];
        char c2 = (i + 1) < len ? str[i + 1] : 0;
        if (c == '"' || c == '\\' || (c == '#' && c2 == '{')) {
            out->append_char(env, '\\');
            out->append_char(env, c);
        } else if (c == '\n') {
            out->append(env, "\\n");
        } else if (c == '\t') {
            out->append(env, "\\t");
        } else if ((int)c < 32) {
            switch (m_encoding) {
            case Encoding::UTF_8: {
                char buf[7];
                snprintf(buf, 7, "\\u%04llX", (long long)c);
                out->append(env, buf);
                break;
            }
            case Encoding::ASCII_8BIT: {
                char buf[5];
                snprintf(buf, 5, "\\x%02llX", (long long)c);
                out->append(env, buf);
                break;
            }
            }
        } else {
            out->append_char(env, c);
        }
    }
    out->append_char(env, '"');
    return out;
}

StringValue *StringValue::successive(Env *env) {
    auto str = m_string.successive();
    return new StringValue { str };
}

bool StringValue::start_with(Env *env, ValuePtr needle) {
    nat_int_t i = index_int(env, needle, 0);
    return i == 0;
}

bool StringValue::end_with(Env *env, ValuePtr needle) {
    needle->assert_type(env, Value::Type::String, "String");
    if (length() < needle->as_string()->length())
        return false;
    auto from_end = new StringValue { c_str() + length() - needle->as_string()->length() };
    nat_int_t i = from_end->index_int(env, needle, 0);
    return i == 0;
}

ValuePtr StringValue::index(Env *env, ValuePtr needle) {
    return index(env, needle, 0);
}

ValuePtr StringValue::index(Env *env, ValuePtr needle, size_t start) {
    auto byte_index = index_int(env, needle, start);
    if (byte_index == -1) {
        return NilValue::the();
    }
    size_t byte_index_size_t = static_cast<size_t>(byte_index);
    size_t char_index = 0, i = 0;
    char buffer[5];
    while (next_char(env, buffer, &i)) {
        if (i > byte_index_size_t)
            return IntegerValue::from_size_t(env, char_index);
        char_index++;
    }
    return ValuePtr::integer(0);
}

nat_int_t StringValue::index_int(Env *env, ValuePtr needle, size_t start) {
    needle->assert_type(env, Value::Type::String, "String");
    const char *ptr = strstr(c_str() + start, needle->as_string()->c_str());
    if (ptr == nullptr)
        return -1;
    return ptr - c_str();
}

ValuePtr StringValue::initialize(Env *env, ValuePtr arg) {
    if (arg) {
        arg->assert_type(env, Value::Type::String, "String");
        set_str(arg->as_string()->c_str());
    }
    return this;
}

ValuePtr StringValue::ltlt(Env *env, ValuePtr arg) {
    this->assert_not_frozen(env);
    if (arg->is_string()) {
        append(env, arg->as_string());
    } else {
        ValuePtr str_obj = arg.send(env, SymbolValue::intern("to_s"));
        str_obj->assert_type(env, Value::Type::String, "String");
        append(env, str_obj->as_string());
    }
    return this;
}

ValuePtr StringValue::add(Env *env, ValuePtr arg) {
    const char *str;
    if (arg->is_string()) {
        str = arg->as_string()->c_str();
    } else {
        StringValue *str_obj = arg.send(env, SymbolValue::intern("to_s"))->as_string();
        str_obj->assert_type(env, Value::Type::String, "String");
        str = str_obj->c_str();
    }
    StringValue *new_string = new StringValue { c_str() };
    new_string->append(env, str);
    return new_string;
}

ValuePtr StringValue::mul(Env *env, ValuePtr arg) {
    arg->assert_type(env, Value::Type::Integer, "Integer");
    StringValue *new_string = new StringValue { "" };
    for (nat_int_t i = 0; i < arg->as_integer()->to_nat_int_t(); i++) {
        new_string->append(env, this);
    }
    return new_string;
}

ValuePtr StringValue::cmp(Env *env, ValuePtr other) {
    if (other->type() != Value::Type::String) return NilValue::the();
    auto *str = c_str();
    auto *other_str = other->as_string()->c_str();
    size_t other_length = other->as_string()->length();
    for (size_t i = 0; i < length(); ++i) {
        if (i >= other_length)
            return ValuePtr::integer(1);
        if (str[i] < other_str[i])
            return ValuePtr::integer(-1);
        else if (str[i] > other_str[i])
            return ValuePtr::integer(1);
    }
    if (length() < other_length)
        return ValuePtr::integer(-1);
    return ValuePtr::integer(0);
}

ValuePtr StringValue::eqtilde(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Regexp, "Regexp");
    return other->as_regexp()->eqtilde(env, this);
}

ValuePtr StringValue::match(Env *env, ValuePtr other) {
    other->assert_type(env, Value::Type::Regexp, "Regexp");
    return other->as_regexp()->match(env, this);
}

ValuePtr StringValue::ord(Env *env) {
    size_t index = 0;
    char buffer[5];
    if (!next_char(env, buffer, &index))
        env->raise("ArgumentError", "empty string");
    auto c = new StringValue { buffer, m_encoding };
    assert(c->length() > 0);
    unsigned int code;
    const char *str = c->c_str();
    switch (c->length()) {
    case 0:
        NAT_UNREACHABLE();
    case 1:
        code = (unsigned char)str[0];
        break;
    case 2:
        code = (((unsigned char)str[0] ^ 0xC0) << 6) + (((unsigned char)str[1] ^ 0x80) << 0);
        break;
    case 3:
        code = (((unsigned char)str[0] ^ 0xE0) << 12) + (((unsigned char)str[1] ^ 0x80) << 6) + (((unsigned char)str[2] ^ 0x80) << 0);
        break;
    case 4:
        code = (((unsigned char)str[0] ^ 0xF0) << 18) + (((unsigned char)str[1] ^ 0x80) << 12) + (((unsigned char)str[2] ^ 0x80) << 6) + (((unsigned char)str[3] ^ 0x80) << 0);
        break;
    default:
        NAT_UNREACHABLE();
    }
    return ValuePtr::integer(code);
}

ValuePtr StringValue::bytes(Env *env) {
    ArrayValue *ary = new ArrayValue {};
    for (size_t i = 0; i < length(); i++) {
        ary->push(ValuePtr::integer(c_str()[i]));
    }
    return ary;
}

ValuePtr StringValue::size(Env *env) {
    size_t index = 0;
    size_t char_count = 0;
    char buffer[5];
    while (next_char(env, buffer, &index)) {
        char_count++;
    }
    return IntegerValue::from_size_t(env, char_count);
}

ValuePtr StringValue::encoding(Env *env) {
    ClassValue *Encoding = GlobalEnv::the()->Object()->const_find(env, SymbolValue::intern("Encoding"))->as_class();
    switch (m_encoding) {
    case Encoding::ASCII_8BIT:
        return Encoding->const_find(env, SymbolValue::intern("ASCII_8BIT"));
    case Encoding::UTF_8:
        return Encoding->const_find(env, SymbolValue::intern("UTF_8"));
    }
    NAT_UNREACHABLE();
}

static EncodingValue *find_encoding_by_name(Env *env, const String *name) {
    String *lcase_name = name->lowercase();
    ArrayValue *list = EncodingValue::list(env);
    for (size_t i = 0; i < list->size(); i++) {
        EncodingValue *encoding = (*list)[i]->as_encoding();
        ArrayValue *names = encoding->names(env);
        for (size_t n = 0; n < names->size(); n++) {
            StringValue *name_obj = (*names)[n]->as_string();
            String *name = name_obj->to_low_level_string()->lowercase();
            if (*name == *lcase_name) {
                return encoding;
            }
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name);
}

ValuePtr StringValue::encode(Env *env, ValuePtr encoding) {
    Encoding orig_encoding = m_encoding;
    StringValue *copy = dup(env)->as_string();
    copy->force_encoding(env, encoding);
    ClassValue *Encoding = GlobalEnv::the()->Object()->const_find(env, SymbolValue::intern("Encoding"))->as_class();
    if (orig_encoding == copy->encoding()) {
        return copy;
    } else if (orig_encoding == Encoding::UTF_8 && copy->encoding() == Encoding::ASCII_8BIT) {
        ArrayValue *chars = this->chars(env);
        for (size_t i = 0; i < chars->size(); i++) {
            StringValue *char_obj = (*chars)[i]->as_string();
            if (char_obj->length() > 1) {
                ValuePtr ord = char_obj->ord(env);
                ValuePtr message = StringValue::format(env, "U+{} from UTF-8 to ASCII-8BIT", int_to_hex_string(ord->as_integer()->to_nat_int_t(), true));
                StringValue zero_x { "0X" };
                StringValue blank { "" };
                message = message->as_string()->sub(env, &zero_x, &blank);
                env->raise(Encoding->const_find(env, SymbolValue::intern("UndefinedConversionError"))->as_class(), "{}", message->as_string());
            }
        }
        return copy;
    } else if (orig_encoding == Encoding::ASCII_8BIT && copy->encoding() == Encoding::UTF_8) {
        return copy;
    } else {
        env->raise(Encoding->const_find(env, SymbolValue::intern("ConverterNotFoundError"))->as_class(), "code converter not found");
    }
}

ValuePtr StringValue::force_encoding(Env *env, ValuePtr encoding) {
    switch (encoding->type()) {
    case Value::Type::Encoding:
        set_encoding(encoding->as_encoding()->num());
        break;
    case Value::Type::String:
        set_encoding(find_encoding_by_name(env, encoding->as_string()->to_low_level_string())->num());
        break;
    default:
        env->raise("TypeError", "no implicit conversion of {} into String", encoding->klass()->class_name_or_blank());
    }
    return this;
}

ValuePtr StringValue::ref(Env *env, ValuePtr index_obj) {
    // not sure how we'd handle a string that big anyway
    assert(length() < NAT_INT_MAX);

    if (index_obj->is_integer()) {
        nat_int_t index = index_obj->as_integer()->to_nat_int_t();

        ArrayValue *chars = this->chars(env);
        if (index < 0) {
            index = chars->size() + index;
        }

        if (index < 0 || index >= (nat_int_t)chars->size()) {
            return NilValue::the();
        }
        return (*chars)[index];
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();

        range->begin()->assert_type(env, Value::Type::Integer, "Integer");
        range->begin()->assert_type(env, Value::Type::Integer, "Integer");

        nat_int_t begin = range->begin()->as_integer()->to_nat_int_t();
        nat_int_t end = range->end()->as_integer()->to_nat_int_t();

        ArrayValue *chars = this->chars(env);

        if (begin < 0) begin = chars->size() + begin;
        if (end < 0) end = chars->size() + end;

        if (begin < 0 || end < 0) return NilValue::the();
        size_t u_begin = static_cast<size_t>(begin);
        size_t u_end = static_cast<size_t>(end);
        if (u_begin > chars->size()) return NilValue::the();

        if (!range->exclude_end()) u_end++;

        size_t max = chars->size();
        u_end = u_end > max ? max : u_end;
        StringValue *result = new StringValue {};
        for (size_t i = begin; i < u_end; i++) {
            result->append(env, (*chars)[i]);
        }

        return result;
    }
    index_obj->assert_type(env, Value::Type::Integer, "Integer");
    abort();
}

ValuePtr StringValue::sub(Env *env, ValuePtr find, ValuePtr replacement, Block *block) {
    if (!block && !replacement)
        env->raise("ArgumentError", "wrong number of arguments (given 1, expected 2)");
    if (!block)
        replacement->assert_type(env, Value::Type::String, "String");
    if (find->is_string()) {
        nat_int_t index = this->index_int(env, find->as_string(), 0);
        if (index == -1) {
            return dup(env)->as_string();
        }
        StringValue *out = new StringValue { c_str(), static_cast<size_t>(index) };
        if (block) {
            ValuePtr result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
            result->assert_type(env, Value::Type::String, "String");
            out->append(env, result->as_string());
        } else {
            out->append(env, replacement->as_string());
        }
        out->append(env, &c_str()[index + find->as_string()->length()]);
        return out;
    } else if (find->is_regexp()) {
        if (block) {
            NAT_NOT_YET_IMPLEMENTED("String#sub(/regex/) { block }")
        }
        MatchDataValue *match;
        StringValue *expanded_replacement;
        return regexp_sub(env, find->as_regexp(), replacement->as_string(), &match, &expanded_replacement);
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp)", find->klass()->class_name_or_blank());
    }
}

ValuePtr StringValue::gsub(Env *env, ValuePtr find, ValuePtr replacement_value, Block *block) {
    if (!replacement_value || block) {
        NAT_NOT_YET_IMPLEMENTED("String#gsub(/regex/) { block }")
    }
    replacement_value->assert_type(env, Value::Type::String, "String");
    StringValue *replacement = replacement_value->as_string();
    if (find->is_string()) {
        NAT_NOT_YET_IMPLEMENTED();
    } else if (find->is_regexp()) {
        MatchDataValue *match = nullptr;
        StringValue *expanded_replacement = nullptr;
        StringValue *result = this;
        size_t start_index = 0;
        do {
            match = nullptr;
            result = result->regexp_sub(env, find->as_regexp(), replacement, &match, &expanded_replacement, start_index);
            if (match)
                start_index = match->index(0) + expanded_replacement->length();
        } while (match);
        return result;
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp)", find->klass()->class_name_or_blank());
    }
}

StringValue *StringValue::regexp_sub(Env *env, RegexpValue *find, StringValue *replacement, MatchDataValue **match, StringValue **expanded_replacement, size_t start_index) {
    ValuePtr match_result = find->as_regexp()->match(env, this, start_index);
    if (match_result == NilValue::the())
        return dup(env)->as_string();
    *match = match_result->as_match_data();
    size_t length = (*match)->as_match_data()->group(env, 0)->as_string()->length();
    nat_int_t index = (*match)->as_match_data()->index(0);
    StringValue *out = new StringValue { c_str(), static_cast<size_t>(index) };
    *expanded_replacement = expand_backrefs(env, replacement->as_string(), *match);
    out->append(env, *expanded_replacement);
    out->append(env, &c_str()[index + length]);
    return out;
}

StringValue *StringValue::expand_backrefs(Env *env, StringValue *str, MatchDataValue *match) {
    const char *c_str = str->c_str();
    size_t len = strlen(c_str);
    StringValue *expanded = new StringValue { "" };
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
                expanded->append_char(env, c);
                break;
            // TODO: there are other back references we need to handle, e.g. \&, \', \`, and \+
            default:
                fprintf(stderr, "Unknown backslash reference: \\%c", c);
                abort();
            }
            break;
        default:
            expanded->append_char(env, c);
        }
    }
    return expanded;
}

ValuePtr StringValue::to_i(Env *env, ValuePtr base_obj) {
    int base = 10;
    if (base_obj) {
        base_obj->assert_type(env, Value::Type::Integer, "Integer");
        base = base_obj->as_integer()->to_nat_int_t();
    }
    nat_int_t number = strtoll(c_str(), nullptr, base);
    return ValuePtr::integer(number);
}

ValuePtr StringValue::split(Env *env, ValuePtr splitter, ValuePtr max_count_value) {
    ArrayValue *ary = new ArrayValue {};
    if (!splitter) {
        splitter = new RegexpValue { env, "\\s+" };
    }
    nat_int_t max_count = 0;
    if (max_count_value) {
        max_count_value->assert_type(env, Value::Type::Integer, "Integer");
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
                ary->push(new StringValue { &c_str()[last_index], index - last_index });
                last_index = index + len;
                if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                    ary->push(new StringValue { &c_str()[last_index] });
                    onig_region_free(region, true);
                    return ary;
                }
                result = splitter->as_regexp()->search(c_str(), last_index, region, ONIG_OPTION_NONE);
            } while (result != ONIG_MISMATCH);
            ary->push(new StringValue { &c_str()[last_index] });
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
                ary->push(new StringValue { &c_str()[last_index], u_index - last_index });
                last_index = u_index + splitter->as_string()->length();
                if (max_count > 0 && ary->size() >= static_cast<size_t>(max_count) - 1) {
                    ary->push(new StringValue { &c_str()[last_index] });
                    return ary;
                }
                index = index_int(env, splitter->as_string(), last_index);
            } while (index != -1);
            ary->push(new StringValue { &c_str()[last_index] });
        }
        return ary;
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp))", splitter->klass()->class_name_or_blank());
    }
}

ValuePtr StringValue::ljust(Env *env, ValuePtr length_obj, ValuePtr pad_obj) {
    length_obj->assert_type(env, Value::Type::Integer, "Integer");
    size_t length = length_obj->as_integer()->to_nat_int_t() < 0 ? 0 : length_obj->as_integer()->to_nat_int_t();
    StringValue *padstr;
    if (pad_obj) {
        pad_obj->assert_type(env, Value::Type::String, "String");
        padstr = pad_obj->as_string();
    } else {
        padstr = new StringValue { " " };
    }
    StringValue *copy = dup(env)->as_string();
    while (copy->length() < length) {
        bool truncate = copy->length() + padstr->length() > length;
        copy->append(env, padstr);
        if (truncate) {
            copy->truncate(length);
        }
    }
    return copy;
}

ValuePtr StringValue::strip(Env *env) {
    if (length() == 0)
        return new StringValue {};
    assert(length() < NAT_INT_MAX);
    nat_int_t first_char, last_char;
    nat_int_t len = static_cast<nat_int_t>(length());
    for (first_char = 0; first_char < len; first_char++) {
        char c = c_str()[first_char];
        if (c != ' ' && c != '\t' && c != '\n')
            break;
    }
    for (last_char = len - 1; last_char >= 0; last_char--) {
        char c = c_str()[last_char];
        if (c != ' ' && c != '\t' && c != '\n')
            break;
    }
    if (last_char < 0) {
        return new StringValue {};
    } else {
        size_t new_length = static_cast<size_t>(last_char - first_char + 1);
        return new StringValue { c_str() + first_char, new_length };
    }
}

ValuePtr StringValue::downcase(Env *env) {
    auto ary = chars(env);
    auto str = new StringValue {};
    for (auto c_val : *ary) {
        auto c_str = c_val->as_string();
        if (c_str->bytesize() > 1) {
            str->append(env, c_str);
        } else {
            auto c = c_str->c_str()[0];
            if (c >= 65 && c <= 90) {
                c += 32;
            }
            str->append_char(env, c);
        }
    }
    return str;
}

ValuePtr StringValue::upcase(Env *env) {
    auto ary = chars(env);
    auto str = new StringValue {};
    for (auto c_val : *ary) {
        auto c_str = c_val->as_string();
        if (c_str->bytesize() > 1) {
            str->append(env, c_str);
        } else {
            auto c = c_str->c_str()[0];
            if (c >= 97 && c <= 122) {
                c -= 32;
            }
            str->append_char(env, c);
        }
    }
    return str;
}

ValuePtr StringValue::reverse(Env *env) {
    if (length() == 0)
        return new StringValue {};
    auto ary = new ArrayValue {};
    auto characters = chars(env)->as_array();
    for (size_t i = characters->size() - 1;; i--) {
        ary->push((*characters)[i]);
        if (i == 0) break;
    }
    return ary->join(env, nullptr);
}

void StringValue::prepend_char(Env *env, char c) {
    m_string.prepend_char(c);
}

void StringValue::insert(Env *, size_t position, char c) {
    m_string.insert(position, c);
}

void StringValue::append_char(Env *, char c) {
    m_string.append_char(c);
}

void StringValue::append(Env *, const char *str) {
    m_string.append(str);
}

void StringValue::append(Env *, const StringValue *str) {
    m_string.append(str->c_str());
}

void StringValue::append(Env *, const String *str) {
    m_string.append(str->c_str());
}

void StringValue::append(Env *env, ValuePtr val) {
    if (val->is_string())
        append(env, val->as_string()->c_str());
    else
        append(env, val->inspect_str(env));
}

}
