#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"
#include <ctype.h>

namespace Natalie {

Value *String_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj = alloc(env, self, ValueType::String);
    send(env, obj, "initialize", argc, args, block);
    return obj;
}

Value *String_initialize(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], ValueType::String, "String");
        free(self->str);
        self->str = heap_string(args[0]->str);
        self->str_len = args[0]->str_len;
        self->str_cap = args[0]->str_cap;
    }
    return self;
}

Value *String_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_ARGC(0);
    return self;
}

Value *String_ltlt(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_NOT_FROZEN(self);
    Value *arg = args[0];
    if (NAT_TYPE(arg) == ValueType::String) {
        string_append_string(env, self, arg);
    } else {
        Value *str_obj = send(env, arg, "to_s", 0, NULL, NULL);
        NAT_ASSERT_TYPE(str_obj, ValueType::String, "String");
        string_append_string(env, self, str_obj);
    }
    return self;
}

Value *String_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_ARGC(0);
    Value *out = string(env, "\"");
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

Value *String_add(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    char *str;
    if (NAT_TYPE(arg) == ValueType::String) {
        str = arg->str;
    } else {
        Value *str_obj = send(env, arg, "to_s", 0, NULL, NULL);
        NAT_ASSERT_TYPE(str_obj, ValueType::String, "String");
        str = str_obj->str;
    }
    Value *new_str = string(env, self->str);
    string_append(env, new_str, str);
    return new_str;
}

Value *String_mul(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, ValueType::Integer, "Integer");
    Value *new_str = string(env, "");
    for (long long i = 0; i < NAT_INT_VALUE(arg); i++) {
        string_append_string(env, new_str, self);
    }
    return new_str;
}

Value *String_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (NAT_TYPE(arg) == ValueType::String && strcmp(self->str, arg->str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *String_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (NAT_TYPE(arg) != ValueType::String) return NAT_NIL;
    int diff = strcmp(self->str, arg->str);
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

Value *String_eqtilde(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_TYPE(args[0], ValueType::Regexp, "Regexp");
    return Regexp_eqtilde(env, args[0], 1, &self, block);
}

Value *String_match(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::String);
    NAT_ASSERT_TYPE(args[0], ValueType::Regexp, "Regexp");
    return Regexp_match(env, args[0], 1, &self, block);
}

static void succ_string(Env *env, Value *string, char append_char, char begin_char, char end_char) {
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

Value *String_succ(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *result = string(env, self->str);
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

Value *String_ord(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *chars = string_chars(env, self);
    if (vector_size(&chars->ary) == 0) {
        NAT_RAISE(env, "ArgumentError", "empty string");
    }
    Value *c = static_cast<Value *>(vector_get(&chars->ary, 0));
    assert(NAT_TYPE(c) == ValueType::String);
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

Value *String_bytes(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *ary = array_new(env);
    for (ssize_t i = 0; i < self->str_len; i++) {
        array_push(env, ary, integer(env, self->str[i]));
    }
    return ary;
}

Value *String_chars(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::String);
    return string_chars(env, self);
}

Value *String_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *chars = string_chars(env, self);
    return integer(env, vector_size(&chars->ary));
}

Value *String_encoding(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *Encoding = const_get(env, NAT_OBJECT, "Encoding", true);
    switch (self->encoding) {
    case Encoding::ASCII_8BIT:
        return const_get(env, Encoding, "ASCII_8BIT", true);
    case Encoding::UTF_8:
        return const_get(env, Encoding, "UTF_8", true);
    }
    NAT_UNREACHABLE();
}

static char *lcase_string(char *str) {
    char *lcase_str = heap_string(str);
    for (int i = 0; lcase_str[i]; i++) {
        lcase_str[i] = tolower(lcase_str[i]);
    }
    return lcase_str;
}

static Value *find_encoding_by_name(Env *env, char *name) {
    char *lcase_name = lcase_string(name);
    Value *list = Encoding_list(env, const_get(env, NAT_OBJECT, "Encoding", true), 0, NULL, NULL);
    for (ssize_t i = 0; i < vector_size(&list->ary); i++) {
        Value *encoding = static_cast<Value *>(vector_get(&list->ary, i));
        Value *names = encoding->encoding_names;
        for (ssize_t n = 0; n < vector_size(&names->ary); n++) {
            Value *name_obj = static_cast<Value *>(vector_get(&names->ary, n));
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

Value *String_encode(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::String);
    Encoding orig_encoding = self->encoding;
    Value *copy = dup(env, self);
    String_force_encoding(env, copy, argc, args, NULL);
    Value *Encoding = const_get(env, NAT_OBJECT, "Encoding", true);
    if (orig_encoding == copy->encoding) {
        return copy;
    } else if (orig_encoding == Encoding::UTF_8 && copy->encoding == Encoding::ASCII_8BIT) {
        Value *chars = string_chars(env, self);
        for (ssize_t i = 0; i < vector_size(&chars->ary); i++) {
            Value *char_obj = static_cast<Value *>(vector_get(&chars->ary, i));
            assert(NAT_TYPE(char_obj) == ValueType::String);
            if (char_obj->str_len > 1) {
                Value *ord = String_ord(env, char_obj, 0, NULL, NULL);
                Value *message = sprintf(env, "U+%X from UTF-8 to ASCII-8BIT", NAT_INT_VALUE(ord));
                Value *sub_args[2] = { string(env, "0X"), string(env, "") };
                message = String_sub(env, message, 2, sub_args, NULL);
                raise(env, const_get(env, Encoding, "UndefinedConversionError", true), "%S", message);
                abort();
            }
        }
        return copy;
    } else if (orig_encoding == Encoding::ASCII_8BIT && copy->encoding == Encoding::UTF_8) {
        return copy;
    } else {
        raise(env, const_get(env, Encoding, "ConverterNotFoundError", true), "code converter not found");
        abort();
    }
}

Value *String_force_encoding(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *encoding = args[0];
    switch (NAT_TYPE(encoding)) {
    case ValueType::Encoding:
        self->encoding = encoding->encoding_num;
        break;
    case ValueType::String:
        self->encoding = find_encoding_by_name(env, encoding->str)->encoding_num;
        break;
    default:
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into %s", NAT_OBJ_CLASS(encoding)->class_name, "String");
    }
    return self;
}

Value *String_ref(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *index_obj = args[0];
    NAT_ASSERT_TYPE(index_obj, ValueType::Integer, "Integer");
    int64_t index = NAT_INT_VALUE(index_obj);

    // not sure how we'd handle that given a 64-bit signed int for an index,
    // not to mention that a string that long would be insane to index into
    assert(self->str_len < INT64_MAX);

    Value *chars = string_chars(env, self);
    if (index < 0) {
        index = vector_size(&chars->ary) + index;
    }

    if (index < 0 || index >= (int64_t)vector_size(&chars->ary)) {
        return NAT_NIL;
    }
    return static_cast<Value *>(vector_get(&chars->ary, index));
}

// FIXME: this is dirty quick solution, a slow algorithm and doesn't account for string encoding
static ssize_t str_index(Value *str, Value *find, ssize_t start) {
    for (ssize_t i = start; i < str->str_len; i++) {
        if (strncmp(&str->str[i], find->str, find->str_len) == 0) {
            return i;
        }
    }
    return -1;
}

Value *String_index(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *find = args[0];
    NAT_ASSERT_TYPE(find, ValueType::String, "String");
    ssize_t index = str_index(self, find, 0);
    if (index == -1) {
        return NAT_NIL;
    }
    return integer(env, index);
}

Value *String_sub(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *sub = args[0];
    Value *repl = args[1];
    NAT_ASSERT_TYPE(repl, ValueType::String, "String");
    if (NAT_TYPE(sub) == ValueType::String) {
        Value *index_obj = String_index(env, self, 1, &sub, NULL);
        if (index_obj == NAT_NIL) {
            return dup(env, self);
        }
        int64_t index = NAT_INT_VALUE(index_obj);
        Value *out = string(env, self->str);
        out->str[index] = 0;
        out->str_len = index;
        string_append_string(env, out, repl);
        string_append(env, out, &self->str[index + sub->str_len]);
        return out;
    } else if (NAT_TYPE(sub) == ValueType::Regexp) {
        Value *match = Regexp_match(env, sub, 1, &self, NULL);
        if (match == NAT_NIL) {
            return dup(env, self);
        }
        Value *match_str = MatchData_to_s(env, match, 0, NULL, NULL);
        int64_t index = match->matchdata_region->beg[0];
        Value *out = string(env, self->str);
        out->str[index] = 0;
        out->str_len = index;
        string_append_string(env, out, repl);
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

Value *String_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    assert(NAT_TYPE(self) == ValueType::String);
    int base = 10;
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], ValueType::Integer, "Integer");
        base = NAT_INT_VALUE(args[0]);
    }
    long long number = str_to_i(self->str, base);
    return integer(env, number);
}

Value *String_split(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *splitter = args[0];
    Value *ary = array_new(env);
    if (self->str_len == 0) {
        return ary;
    } else if (NAT_TYPE(splitter) == ValueType::Regexp) {
        ssize_t last_index = 0;
        ssize_t index, len;
        OnigRegion *region = onig_region_new();
        unsigned char *str = (unsigned char *)self->str;
        unsigned char *end = str + self->str_len;
        unsigned char *start = str;
        unsigned char *range = end;
        int result = onig_search(splitter->regexp, str, end, start, range, region, ONIG_OPTION_NONE);
        if (result == ONIG_MISMATCH) {
            array_push(env, ary, dup(env, self));
        } else {
            do {
                index = region->beg[0];
                len = region->end[0] - region->beg[0];
                array_push(env, ary, string_n(env, &self->str[last_index], index - last_index));
                last_index = index + len;
                result = onig_search(splitter->regexp, str, end, start + last_index, range, region, ONIG_OPTION_NONE);
            } while (result != ONIG_MISMATCH);
            array_push(env, ary, string(env, &self->str[last_index]));
        }
        onig_region_free(region, true);
        return ary;
    } else if (NAT_TYPE(splitter) == ValueType::String) {
        ssize_t last_index = 0;
        ssize_t index = str_index(self, splitter, 0);
        if (index == -1) {
            array_push(env, ary, dup(env, self));
        } else {
            do {
                array_push(env, ary, string_n(env, &self->str[last_index], index - last_index));
                last_index = index + splitter->str_len;
                index = str_index(self, splitter, last_index);
            } while (index != -1);
            array_push(env, ary, string(env, &self->str[last_index]));
        }
        return ary;
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Regexp))", NAT_OBJ_CLASS(splitter)->class_name);
    }
}

Value *String_ljust(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    assert(NAT_TYPE(self) == ValueType::String);
    Value *length_obj = args[0];
    NAT_ASSERT_TYPE(length_obj, ValueType::Integer, "Integer");
    ssize_t length = NAT_INT_VALUE(length_obj) < 0 ? 0 : NAT_INT_VALUE(length_obj);
    Value *padstr;
    if (argc > 1) {
        padstr = args[1];
        NAT_ASSERT_TYPE(padstr, ValueType::String, "String");
    } else {
        padstr = string(env, " ");
    }
    Value *copy = dup(env, self);
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
