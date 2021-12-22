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

void StringObject::raise_encoding_invalid_byte_sequence_error(Env *env, size_t index) const {
    StringObject *message = format(env, "invalid byte sequence at index {} in string of size {} (string not long enough)", index, length());
    ClassObject *InvalidByteSequenceError = find_nested_const(env, { "Encoding"_s, "InvalidByteSequenceError"_s })->as_class();
    ExceptionObject *exception = new ExceptionObject { InvalidByteSequenceError, message };
    env->raise_exception(exception);
}

char *StringObject::next_char(Env *env, char *buffer, size_t *index) {
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

Value StringObject::each_char(Env *env, Block *block) {
    if (!block)
        return send(env, "enum_for"_s, { "each_char"_s });

    size_t index = 0;
    char buffer[5];
    while (next_char(env, buffer, &index)) {
        auto c = new StringObject { buffer, m_encoding };
        Value args[] = { c };
        NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 1, args, nullptr);
    }
    return NilObject::the();
}

ArrayObject *StringObject::chars(Env *env) {
    ArrayObject *ary = new ArrayObject {};
    size_t index = 0;
    char buffer[5];
    while (next_char(env, buffer, &index)) {
        auto c = new StringObject { buffer, m_encoding };
        ary->push(c);
    }
    return ary;
}

SymbolObject *StringObject::to_symbol(Env *env) const {
    return SymbolObject::intern(c_str());
}

Value StringObject::to_sym(Env *env) const {
    return to_symbol(env);
}

StringObject *StringObject::inspect(Env *env) {
    size_t len = length();
    const char *str = m_string.c_str();
    StringObject *out = new StringObject { "\"" };
    for (size_t i = 0; i < len; i++) {
        unsigned char c = str[i];
        char c2 = (i + 1) < len ? str[i + 1] : 0;
        if (c == '"' || c == '\\' || (c == '#' && c2 == '{')) {
            out->append_char('\\');
            out->append_char(c);
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

bool StringObject::start_with(Env *env, Value needle) const {
    nat_int_t i = index_int(env, needle, 0);
    return i == 0;
}

bool StringObject::end_with(Env *env, Value needle) const {
    needle->assert_type(env, Object::Type::String, "String");
    if (length() < needle->as_string()->length())
        return false;
    auto from_end = new StringObject { c_str() + length() - needle->as_string()->length() };
    nat_int_t i = from_end->index_int(env, needle, 0);
    return i == 0;
}

Value StringObject::index(Env *env, Value needle) {
    return index(env, needle, 0);
}

Value StringObject::index(Env *env, Value needle, size_t start) {
    auto byte_index = index_int(env, needle, start);
    if (byte_index == -1) {
        return NilObject::the();
    }
    size_t byte_index_size_t = static_cast<size_t>(byte_index);
    size_t char_index = 0, i = 0;
    char buffer[5];
    while (next_char(env, buffer, &i)) {
        if (i > byte_index_size_t)
            return IntegerObject::from_size_t(env, char_index);
        char_index++;
    }
    return Value::integer(0);
}

nat_int_t StringObject::index_int(Env *env, Value needle, size_t start) const {
    needle->assert_type(env, Object::Type::String, "String");
    const char *ptr = strstr(c_str() + start, needle->as_string()->c_str());
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

Value StringObject::ltlt(Env *env, Value arg) {
    this->assert_not_frozen(env);
    if (arg->is_string()) {
        append(env, arg->as_string());
    } else {
        Value str_obj = arg.send(env, "to_s"_s);
        str_obj->assert_type(env, Object::Type::String, "String");
        append(env, str_obj->as_string());
    }
    return this;
}

Value StringObject::add(Env *env, Value arg) const {
    const char *str;
    if (arg->is_string()) {
        str = arg->as_string()->c_str();
    } else {
        StringObject *str_obj = arg.send(env, "to_s"_s)->as_string();
        str_obj->assert_type(env, Object::Type::String, "String");
        str = str_obj->c_str();
    }
    StringObject *new_string = new StringObject { c_str() };
    new_string->append(env, str);
    return new_string;
}

Value StringObject::mul(Env *env, Value arg) const {
    arg->assert_type(env, Object::Type::Integer, "Integer");
    StringObject *new_string = new StringObject { "" };
    for (nat_int_t i = 0; i < arg->as_integer()->to_nat_int_t(); i++) {
        new_string->append(env, this);
    }
    return new_string;
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

Value StringObject::ord(Env *env) {
    size_t index = 0;
    char buffer[5];
    if (!next_char(env, buffer, &index))
        env->raise("ArgumentError", "empty string");
    auto c = new StringObject { buffer, m_encoding };
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
    return Value::integer(code);
}

Value StringObject::bytes(Env *env) const {
    ArrayObject *ary = new ArrayObject { length() };
    for (size_t i = 0; i < length(); i++) {
        unsigned char c = c_str()[i];
        ary->push(Value::integer(c));
    }
    return ary;
}

Value StringObject::size(Env *env) {
    size_t index = 0;
    size_t char_count = 0;
    char buffer[5];
    while (next_char(env, buffer, &index)) {
        char_count++;
    }
    return IntegerObject::from_size_t(env, char_count);
}

Value StringObject::encoding(Env *env) {
    ClassObject *Encoding = find_top_level_const(env, "Encoding"_s)->as_class();
    switch (m_encoding) {
    case Encoding::ASCII_8BIT:
        return Encoding->const_find(env, "ASCII_8BIT"_s);
    case Encoding::UTF_8:
        return Encoding->const_find(env, "UTF_8"_s);
    }
    NAT_UNREACHABLE();
}

static EncodingObject *find_encoding_by_name(Env *env, const String *name) {
    String *lcase_name = name->lowercase();
    ArrayObject *list = EncodingObject::list(env);
    for (size_t i = 0; i < list->size(); i++) {
        EncodingObject *encoding = (*list)[i]->as_encoding();
        ArrayObject *names = encoding->names(env);
        for (size_t n = 0; n < names->size(); n++) {
            StringObject *name_obj = (*names)[n]->as_string();
            String *name = name_obj->to_low_level_string()->lowercase();
            if (*name == *lcase_name) {
                return encoding;
            }
        }
    }
    env->raise("ArgumentError", "unknown encoding name - {}", name);
}

Value StringObject::encode(Env *env, Value encoding) {
    Encoding orig_encoding = m_encoding;
    StringObject *copy = dup(env)->as_string();
    copy->force_encoding(env, encoding);
    ClassObject *Encoding = find_top_level_const(env, "Encoding"_s)->as_class();
    if (orig_encoding == copy->encoding()) {
        return copy;
    } else if (orig_encoding == Encoding::UTF_8 && copy->encoding() == Encoding::ASCII_8BIT) {
        ArrayObject *chars = this->chars(env);
        for (size_t i = 0; i < chars->size(); i++) {
            StringObject *char_obj = (*chars)[i]->as_string();
            if (char_obj->length() > 1) {
                Value ord = char_obj->ord(env);
                Value message = StringObject::format(env, "U+{} from UTF-8 to ASCII-8BIT", int_to_hex_string(ord->as_integer()->to_nat_int_t(), true));
                StringObject zero_x { "0X" };
                StringObject blank { "" };
                message = message->as_string()->sub(env, &zero_x, &blank);
                env->raise(Encoding->const_find(env, "UndefinedConversionError"_s)->as_class(), "{}", message->as_string());
            }
        }
        return copy;
    } else if (orig_encoding == Encoding::ASCII_8BIT && copy->encoding() == Encoding::UTF_8) {
        return copy;
    } else {
        env->raise(Encoding->const_find(env, "ConverterNotFoundError"_s)->as_class(), "code converter not found");
    }
}

Value StringObject::force_encoding(Env *env, Value encoding) {
    switch (encoding->type()) {
    case Object::Type::Encoding:
        set_encoding(encoding->as_encoding()->num());
        break;
    case Object::Type::String:
        set_encoding(find_encoding_by_name(env, encoding->as_string()->to_low_level_string())->num());
        break;
    default:
        env->raise("TypeError", "no implicit conversion of {} into String", encoding->klass()->class_name_or_blank());
    }
    return this;
}

Value StringObject::ref(Env *env, Value index_obj) {
    // not sure how we'd handle a string that big anyway
    assert(length() < NAT_INT_MAX);

    if (index_obj->is_integer()) {
        nat_int_t index = index_obj->as_integer()->to_nat_int_t();

        ArrayObject *chars = this->chars(env);
        if (index < 0) {
            index = chars->size() + index;
        }

        if (index < 0 || index >= (nat_int_t)chars->size()) {
            return NilObject::the();
        }
        return (*chars)[index];
    } else if (index_obj->is_range()) {
        RangeObject *range = index_obj->as_range();

        range->begin()->assert_type(env, Object::Type::Integer, "Integer");
        range->begin()->assert_type(env, Object::Type::Integer, "Integer");

        nat_int_t begin = range->begin()->as_integer()->to_nat_int_t();
        nat_int_t end = range->end()->as_integer()->to_nat_int_t();

        ArrayObject *chars = this->chars(env);

        if (begin < 0) begin = chars->size() + begin;
        if (end < 0) end = chars->size() + end;

        if (begin < 0 || end < 0) return NilObject::the();
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

Value StringObject::sub(Env *env, Value find, Value replacement, Block *block) {
    if (!block && !replacement)
        env->raise("ArgumentError", "wrong number of arguments (given 1, expected 2)");
    if (!block)
        replacement->assert_type(env, Object::Type::String, "String");
    if (find->is_string()) {
        nat_int_t index = this->index_int(env, find->as_string(), 0);
        if (index == -1) {
            return dup(env)->as_string();
        }
        StringObject *out = new StringObject { c_str(), static_cast<size_t>(index) };
        if (block) {
            Value result = NAT_RUN_BLOCK_AND_POSSIBLY_BREAK(env, block, 0, nullptr, nullptr);
            result->assert_type(env, Object::Type::String, "String");
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
        MatchDataObject *match;
        StringObject *expanded_replacement;
        return regexp_sub(env, find->as_regexp(), replacement->as_string(), &match, &expanded_replacement);
    } else {
        env->raise("TypeError", "wrong argument type {} (expected Regexp)", find->klass()->class_name_or_blank());
    }
}

Value StringObject::gsub(Env *env, Value find, Value replacement_value, Block *block) {
    if (!replacement_value || block) {
        NAT_NOT_YET_IMPLEMENTED("String#gsub(/regex/) { block }")
    }
    replacement_value->assert_type(env, Object::Type::String, "String");
    StringObject *replacement = replacement_value->as_string();
    if (find->is_string()) {
        NAT_NOT_YET_IMPLEMENTED();
    } else if (find->is_regexp()) {
        MatchDataObject *match = nullptr;
        StringObject *expanded_replacement = nullptr;
        StringObject *result = this;
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

StringObject *StringObject::regexp_sub(Env *env, RegexpObject *find, StringObject *replacement, MatchDataObject **match, StringObject **expanded_replacement, size_t start_index) {
    Value match_result = find->as_regexp()->match(env, this, start_index);
    if (match_result == NilObject::the())
        return dup(env)->as_string();
    *match = match_result->as_match_data();
    size_t length = (*match)->as_match_data()->group(env, 0)->as_string()->length();
    nat_int_t index = (*match)->as_match_data()->index(0);
    StringObject *out = new StringObject { c_str(), static_cast<size_t>(index) };
    *expanded_replacement = expand_backrefs(env, replacement->as_string(), *match);
    out->append(env, *expanded_replacement);
    out->append(env, &c_str()[index + length]);
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
        env->raise("TypeError", "wrong argument type {} (expected Regexp))", splitter->klass()->class_name_or_blank());
    }
}

Value StringObject::ljust(Env *env, Value length_obj, Value pad_obj) {
    length_obj->assert_type(env, Object::Type::Integer, "Integer");
    size_t length = length_obj->as_integer()->to_nat_int_t() < 0 ? 0 : length_obj->as_integer()->to_nat_int_t();
    StringObject *padstr;
    if (pad_obj) {
        pad_obj->assert_type(env, Object::Type::String, "String");
        padstr = pad_obj->as_string();
    } else {
        padstr = new StringObject { " " };
    }
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
    m_string.append(str->c_str());
}

void StringObject::append(Env *, const String *str) {
    m_string.append(str->c_str());
}

void StringObject::append(Env *env, Value val) {
    if (val->is_string())
        append(env, val->as_string()->c_str());
    else
        append(env, val->inspect_str(env));
}

}
