#include "natalie.hpp"
#include "natalie/builtin.hpp"
#include <ctype.h>

namespace Natalie {

Value *String_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj = new StringValue { env, self->as_class() };
    return obj->initialize(env, argc, args, block);
}

Value *String_initialize(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(0, 1);
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
        StringValue *arg = args[0]->as_string();
        free(self->str);
        self->str = strdup(arg->str);
        self->str_len = arg->str_len;
        self->str_cap = arg->str_cap;
    }
    return self;
}

Value *String_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(0);
    return self;
}

Value *String_ltlt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_NOT_FROZEN(self);
    Value *arg = args[0];
    if (arg->is_string()) {
        string_append_string(env, self, arg->as_string());
    } else {
        Value *str_obj = send(env, arg, "to_s", 0, NULL, NULL);
        NAT_ASSERT_TYPE(str_obj, Value::Type::String, "String");
        string_append_string(env, self, str_obj->as_string());
    }
    return self;
}

Value *String_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(0);
    StringValue *out = string(env, "\"");
    for (ssize_t i = 0; i < self->str_len; i++) {
        char c = self->str[i];
        if (c == '"' || c == '\\' || c == '#') {
            string_append_char(env, out, '\\');
            string_append_char(env, out, c);
        } else if (c == '\n') {
            string_append(env, out, "\\n");
        } else if (c == '\t') {
            string_append(env, out, "\\t");
        } else {
            string_append_char(env, out, c);
        }
    }
    string_append_char(env, out, '"');
    return out;
}

Value *String_add(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    char *str;
    if (args[0]->is_string()) {
        str = args[0]->as_string()->str;
    } else {
        StringValue *str_obj = send(env, args[0], "to_s", 0, NULL, NULL)->as_string();
        NAT_ASSERT_TYPE(str_obj, Value::Type::String, "String");
        str = str_obj->str;
    }
    StringValue *new_str = string(env, self->str);
    string_append(env, new_str, str);
    return new_str;
}

Value *String_mul(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    StringValue *new_str = string(env, "");
    for (long long i = 0; i < NAT_INT_VALUE(arg); i++) {
        string_append_string(env, new_str, self);
    }
    return new_str;
}

Value *String_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (arg->is_string() && strcmp(self->str, arg->as_string()->str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *String_cmp(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (NAT_TYPE(arg) != Value::Type::String) return NAT_NIL;
    int diff = strcmp(self->str, arg->as_string()->str);
    int result;
    if (diff < 0) {
        result = -1;
    } else if (diff > 0) {
        result = 1;
    } else {
        result = 0;
    }
    return integer(env, result);
}

Value *String_eqtilde(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::Regexp, "Regexp");
    return Regexp_eqtilde(env, args[0], 1, &self_value, block);
}

Value *String_match(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::Regexp, "Regexp");
    return Regexp_match(env, args[0], 1, &self_value, block);
}

static void succ_string(Env *env, StringValue *string, char append_char, char begin_char, char end_char) {
    ssize_t index = string->str_len - 1;
    char last_char = string->str[index];
    while (last_char == end_char) {
        string->str[index] = begin_char;
        last_char = string->str[--index];
    }
    if (index == -1) {
        string_append_char(env, string, append_char);
    } else {
        string->str[index]++;
    }
}

Value *String_succ(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    StringValue *self = self_value->as_string();
    StringValue *result = string(env, self->str);
    ssize_t index = self->str_len - 1;
    char last_char = self->str[index];
    if (last_char == 'z') {
        succ_string(env, result, 'a', 'a', 'z');
    } else if (last_char == 'Z') {
        succ_string(env, result, 'A', 'A', 'Z');
    } else if (last_char == '9') {
        succ_string(env, result, '0', '1', '9');
    } else {
        result->str[index]++;
    }
    return result;
}

Value *String_ord(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    ArrayValue *chars = string_chars(env, self_value->as_string());
    if (vector_size(&chars->ary) == 0) {
        NAT_RAISE(env, "ArgumentError", "empty string");
    }
    StringValue *c = static_cast<StringValue *>(vector_get(&chars->ary, 0));
    assert(c->str_len > 0);
    unsigned int code;
    switch (c->str_len) {
    case 0:
        NAT_UNREACHABLE();
    case 1:
        code = (unsigned char)c->str[0];
        return integer(env, code);
    case 2:
        code = (((unsigned char)c->str[0] ^ 0xC0) << 6) + (((unsigned char)c->str[1] ^ 0x80) << 0);
        return integer(env, code);
    case 3:
        code = (((unsigned char)c->str[0] ^ 0xE0) << 12) + (((unsigned char)c->str[1] ^ 0x80) << 6) + (((unsigned char)c->str[2] ^ 0x80) << 0);
        return integer(env, code);
    case 4:
        code = (((unsigned char)c->str[0] ^ 0xF0) << 18) + (((unsigned char)c->str[1] ^ 0x80) << 12) + (((unsigned char)c->str[2] ^ 0x80) << 6) + (((unsigned char)c->str[3] ^ 0x80) << 0);
        return integer(env, code);
    default:
        NAT_UNREACHABLE();
    }
}

Value *String_bytes(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    StringValue *self = self_value->as_string();
    Value *ary = array_new(env);
    for (ssize_t i = 0; i < self->str_len; i++) {
        array_push(env, ary, integer(env, self->str[i]));
    }
    return ary;
}

Value *String_chars(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    StringValue *self = self_value->as_string();
    return string_chars(env, self);
}

Value *String_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    StringValue *self = self_value->as_string();
    ArrayValue *chars = string_chars(env, self);
    return integer(env, vector_size(&chars->ary));
}

Value *String_encoding(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    StringValue *self = self_value->as_string();
    ClassValue *Encoding = NAT_OBJECT->const_get(env, "Encoding", true)->as_class();
    switch (self->encoding) {
    case Encoding::ASCII_8BIT:
        return Encoding->const_get(env, "ASCII_8BIT", true);
    case Encoding::UTF_8:
        return Encoding->const_get(env, "UTF_8", true);
    }
    NAT_UNREACHABLE();
}

static char *lcase_string(char *str) {
    char *lcase_str = strdup(str);
    for (int i = 0; lcase_str[i]; i++) {
        lcase_str[i] = tolower(lcase_str[i]);
    }
    return lcase_str;
}

static EncodingValue *find_encoding_by_name(Env *env, char *name) {
    char *lcase_name = lcase_string(name);
    ArrayValue *list = Encoding_list(env, NAT_OBJECT->const_get(env, "Encoding", true), 0, NULL, NULL)->as_array();
    for (ssize_t i = 0; i < vector_size(&list->ary); i++) {
        EncodingValue *encoding = static_cast<EncodingValue *>(vector_get(&list->ary, i));
        ArrayValue *names = encoding->encoding_names;
        for (ssize_t n = 0; n < vector_size(&names->ary); n++) {
            StringValue *name_obj = static_cast<StringValue *>(vector_get(&names->ary, n));
            char *name = lcase_string(name_obj->str);
            if (strcmp(name, lcase_name) == 0) {
                free(name);
                free(lcase_name);
                return encoding;
            }
            free(name);
        }
    }
    free(lcase_name);
    NAT_RAISE(env, "ArgumentError", "unknown encoding name - %s", name);
}

Value *String_encode(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Encoding orig_encoding = self->encoding;
    StringValue *copy = dup(env, self)->as_string();
    String_force_encoding(env, copy, argc, args, NULL);
    ClassValue *Encoding = NAT_OBJECT->const_get(env, "Encoding", true)->as_class();
    if (orig_encoding == copy->encoding) {
        return copy;
    } else if (orig_encoding == Encoding::UTF_8 && copy->encoding == Encoding::ASCII_8BIT) {
        ArrayValue *chars = string_chars(env, self);
        for (ssize_t i = 0; i < vector_size(&chars->ary); i++) {
            StringValue *char_obj = static_cast<StringValue *>(vector_get(&chars->ary, i));
            if (char_obj->str_len > 1) {
                Value *ord = String_ord(env, char_obj, 0, NULL, NULL);
                Value *message = sprintf(env, "U+%X from UTF-8 to ASCII-8BIT", NAT_INT_VALUE(ord));
                Value *sub_args[2] = { string(env, "0X"), string(env, "") };
                message = String_sub(env, message, 2, sub_args, NULL);
                env->raise(Encoding->const_get(env, "UndefinedConversionError", true)->as_class(), "%S", message);
                abort();
            }
        }
        return copy;
    } else if (orig_encoding == Encoding::ASCII_8BIT && copy->encoding == Encoding::UTF_8) {
        return copy;
    } else {
        env->raise(Encoding->const_get(env, "ConverterNotFoundError", true)->as_class(), "code converter not found");
        abort();
    }
}

Value *String_force_encoding(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Value *encoding = args[0];
    switch (NAT_TYPE(encoding)) {
    case Value::Type::Encoding:
        self->encoding = encoding->as_encoding()->encoding_num;
        break;
    case Value::Type::String:
        self->encoding = find_encoding_by_name(env, encoding->as_string()->str)->encoding_num;
        break;
    default:
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into %s", NAT_OBJ_CLASS(encoding)->class_name, "String");
    }
    return self;
}

Value *String_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Value *index_obj = args[0];
    NAT_ASSERT_TYPE(index_obj, Value::Type::Integer, "Integer");
    int64_t index = NAT_INT_VALUE(index_obj);

    // not sure how we'd handle that given a 64-bit signed int for an index,
    // not to mention that a string that long would be insane to index into
    assert(self->str_len < INT64_MAX);

    ArrayValue *chars = string_chars(env, self);
    if (index < 0) {
        index = vector_size(&chars->ary) + index;
    }

    if (index < 0 || index >= (int64_t)vector_size(&chars->ary)) {
        return NAT_NIL;
    }
    return static_cast<Value *>(vector_get(&chars->ary, index));
}

// FIXME: this is dirty quick solution, a slow algorithm and doesn't account for string encoding
static ssize_t str_index(StringValue *str, StringValue *find, ssize_t start) {
    for (ssize_t i = start; i < str->str_len; i++) {
        if (strncmp(&str->str[i], find->str, find->str_len) == 0) {
            return i;
        }
    }
    return -1;
}

Value *String_index(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Value *find = args[0];
    NAT_ASSERT_TYPE(find, Value::Type::String, "String");
    ssize_t index = str_index(self, find->as_string(), 0);
    if (index == -1) {
        return NAT_NIL;
    }
    return integer(env, index);
}

Value *String_sub(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    StringValue *self = self_value->as_string();
    Value *sub = args[0];
    Value *repl = args[1];
    NAT_ASSERT_TYPE(repl, Value::Type::String, "String");
    if (sub->is_string()) {
        Value *index_obj = String_index(env, self, 1, &sub, NULL);
        if (index_obj == NAT_NIL) {
            return dup(env, self);
        }
        int64_t index = NAT_INT_VALUE(index_obj);
        StringValue *out = string(env, self->str);
        out->str[index] = 0;
        out->str_len = index;
        string_append_string(env, out, repl->as_string());
        string_append(env, out, &self->str[index + sub->as_string()->str_len]);
        return out;
    } else if (sub->is_regexp()) {
        Value *match = Regexp_match(env, sub, 1, &self_value, NULL);
        if (match == NAT_NIL) {
            return dup(env, self);
        }
        StringValue *match_str = MatchData_to_s(env, match, 0, NULL, NULL)->as_string();
        int64_t index = match->as_match_data()->matchdata_region->beg[0];
        StringValue *out = string(env, self->str);
        out->str[index] = 0;
        out->str_len = index;
        string_append_string(env, out, repl->as_string());
        string_append(env, out, &self->str[index + match_str->str_len]);
        return out;
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Regexp)", NAT_OBJ_CLASS(sub)->class_name);
    }
}

static uint64_t str_to_i(char *str, int base) {
    int64_t number = 0;
    for (; *str; str++) {
        char digit;
        if (*str >= 'a' && *str <= 'f') {
            digit = *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            digit = *str - 'A' + 10;
        } else {
            digit = *str - '0';
        }
        if (digit >= 0 && digit < base) {
            number = (number * base) + digit;
        } else if (number != 0) {
            break;
        }
    }
    return number;
}

Value *String_to_i(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    StringValue *self = self_value->as_string();
    int base = 10;
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
        base = NAT_INT_VALUE(args[0]);
    }
    long long number = str_to_i(self->str, base);
    return integer(env, number);
}

Value *String_split(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Value *splitter = args[0];
    Value *ary = array_new(env);
    if (self->str_len == 0) {
        return ary;
    } else if (splitter->is_regexp()) {
        ssize_t last_index = 0;
        ssize_t index, len;
        OnigRegion *region = onig_region_new();
        unsigned char *str = (unsigned char *)self->str;
        unsigned char *end = str + self->str_len;
        unsigned char *start = str;
        unsigned char *range = end;
        int result = onig_search(splitter->as_regexp()->regexp, str, end, start, range, region, ONIG_OPTION_NONE);
        if (result == ONIG_MISMATCH) {
            array_push(env, ary, dup(env, self));
        } else {
            do {
                index = region->beg[0];
                len = region->end[0] - region->beg[0];
                array_push(env, ary, string_n(env, &self->str[last_index], index - last_index));
                last_index = index + len;
                result = onig_search(splitter->as_regexp()->regexp, str, end, start + last_index, range, region, ONIG_OPTION_NONE);
            } while (result != ONIG_MISMATCH);
            array_push(env, ary, string(env, &self->str[last_index]));
        }
        onig_region_free(region, true);
        return ary;
    } else if (splitter->is_string()) {
        ssize_t last_index = 0;
        ssize_t index = str_index(self, splitter->as_string(), 0);
        if (index == -1) {
            array_push(env, ary, dup(env, self));
        } else {
            do {
                array_push(env, ary, string_n(env, &self->str[last_index], index - last_index));
                last_index = index + splitter->as_string()->str_len;
                index = str_index(self, splitter->as_string(), last_index);
            } while (index != -1);
            array_push(env, ary, string(env, &self->str[last_index]));
        }
        return ary;
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Regexp))", NAT_OBJ_CLASS(splitter)->class_name);
    }
}

Value *String_ljust(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    StringValue *self = self_value->as_string();
    Value *length_obj = args[0];
    NAT_ASSERT_TYPE(length_obj, Value::Type::Integer, "Integer");
    ssize_t length = NAT_INT_VALUE(length_obj) < 0 ? 0 : NAT_INT_VALUE(length_obj);
    StringValue *padstr;
    if (argc > 1) {
        NAT_ASSERT_TYPE(args[1], Value::Type::String, "String");
        padstr = args[1]->as_string();
    } else {
        padstr = string(env, " ");
    }
    StringValue *copy = dup(env, self)->as_string();
    while (copy->str_len < length) {
        bool truncate = copy->str_len + padstr->str_len > length;
        string_append_string(env, copy, padstr);
        if (truncate) {
            copy->str[length] = 0;
            copy->str_len = length;
        }
    }
    return copy;
}

}
