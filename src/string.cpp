#include "builtin.hpp"
#include "gc.hpp"
#include "natalie.hpp"
#include <ctype.h>

namespace Natalie {

NatObject *String_new(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *obj = alloc(env, self, NAT_VALUE_STRING);
    send(env, obj, "initialize", argc, args, block);
    return obj;
}

NatObject *String_initialize(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
        free(self->str);
        self->str = heap_string(args[0]->str);
        self->str_len = args[0]->str_len;
        self->str_cap = args[0]->str_cap;
    }
    return self;
}

NatObject *String_to_s(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(0);
    return self;
}

NatObject *String_ltlt(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_STRING) {
        string_append_string(env, self, arg);
    } else {
        NatObject *str_obj = send(env, arg, "to_s", 0, NULL, NULL);
        NAT_ASSERT_TYPE(str_obj, NAT_VALUE_STRING, "String");
        string_append_string(env, self, str_obj);
    }
    return self;
}

NatObject *String_inspect(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(0);
    NatObject *out = string(env, "\"");
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

NatObject *String_add(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    char *str;
    if (NAT_TYPE(arg) == NAT_VALUE_STRING) {
        str = arg->str;
    } else {
        NatObject *str_obj = send(env, arg, "to_s", 0, NULL, NULL);
        NAT_ASSERT_TYPE(str_obj, NAT_VALUE_STRING, "String");
        str = str_obj->str;
    }
    NatObject *new_str = string(env, self->str);
    string_append(env, new_str, str);
    return new_str;
}

NatObject *String_mul(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    NatObject *new_str = string(env, "");
    for (long long i = 0; i < NAT_INT_VALUE(arg); i++) {
        string_append_string(env, new_str, self);
    }
    return new_str;
}

NatObject *String_eqeq(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_STRING && strcmp(self->str, arg->str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *String_cmp(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) != NAT_VALUE_STRING) return NAT_NIL;
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

NatObject *String_eqtilde(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_REGEXP, "Regexp");
    return Regexp_eqtilde(env, args[0], 1, &self, block);
}

NatObject *String_match(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_REGEXP, "Regexp");
    return Regexp_match(env, args[0], 1, &self, block);
}

static void succ_string(Env *env, NatObject *string, char append_char, char begin_char, char end_char) {
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

NatObject *String_succ(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *result = string(env, self->str);
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

NatObject *String_ord(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NatObject *chars = string_chars(env, self);
    if (vector_size(&chars->ary) == 0) {
        NAT_RAISE(env, "ArgumentError", "empty string");
    }
    NatObject *c = static_cast<NatObject *>(vector_get(&chars->ary, 0));
    assert(NAT_TYPE(c) == NAT_VALUE_STRING);
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

NatObject *String_bytes(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *ary = array_new(env);
    for (ssize_t i = 0; i < self->str_len; i++) {
        array_push(env, ary, integer(env, self->str[i]));
    }
    return ary;
}

NatObject *String_chars(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    return string_chars(env, self);
}

NatObject *String_size(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *chars = string_chars(env, self);
    return integer(env, vector_size(&chars->ary));
}

NatObject *String_encoding(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *Encoding = const_get(env, NAT_OBJECT, "Encoding", true);
    switch (self->encoding) {
    case NAT_ENCODING_ASCII_8BIT:
        return const_get(env, Encoding, "ASCII_8BIT", true);
    case NAT_ENCODING_UTF_8:
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

static NatObject *find_encoding_by_name(Env *env, char *name) {
    char *lcase_name = lcase_string(name);
    NatObject *list = Encoding_list(env, const_get(env, NAT_OBJECT, "Encoding", true), 0, NULL, NULL);
    for (ssize_t i = 0; i < vector_size(&list->ary); i++) {
        NatObject *encoding = static_cast<NatObject *>(vector_get(&list->ary, i));
        NatObject *names = encoding->encoding_names;
        for (ssize_t n = 0; n < vector_size(&names->ary); n++) {
            NatObject *name_obj = static_cast<NatObject *>(vector_get(&names->ary, n));
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

NatObject *String_encode(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    enum NatEncoding orig_encoding = self->encoding;
    NatObject *copy = dup(env, self);
    String_force_encoding(env, copy, argc, args, NULL);
    NatObject *Encoding = const_get(env, NAT_OBJECT, "Encoding", true);
    if (orig_encoding == copy->encoding) {
        return copy;
    } else if (orig_encoding == NAT_ENCODING_UTF_8 && copy->encoding == NAT_ENCODING_ASCII_8BIT) {
        NatObject *chars = string_chars(env, self);
        for (ssize_t i = 0; i < vector_size(&chars->ary); i++) {
            NatObject *char_obj = static_cast<NatObject *>(vector_get(&chars->ary, i));
            assert(NAT_TYPE(char_obj) == NAT_VALUE_STRING);
            if (char_obj->str_len > 1) {
                NatObject *ord = String_ord(env, char_obj, 0, NULL, NULL);
                NatObject *message = sprintf(env, "U+%X from UTF-8 to ASCII-8BIT", NAT_INT_VALUE(ord));
                NatObject *sub_args[2] = { string(env, "0X"), string(env, "") };
                message = String_sub(env, message, 2, sub_args, NULL);
                raise(env, const_get(env, Encoding, "UndefinedConversionError", true), "%S", message);
                abort();
            }
        }
        return copy;
    } else if (orig_encoding == NAT_ENCODING_ASCII_8BIT && copy->encoding == NAT_ENCODING_UTF_8) {
        return copy;
    } else {
        raise(env, const_get(env, Encoding, "ConverterNotFoundError", true), "code converter not found");
        abort();
    }
}

NatObject *String_force_encoding(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *encoding = args[0];
    switch (NAT_TYPE(encoding)) {
    case NAT_VALUE_ENCODING:
        self->encoding = encoding->encoding_num;
        break;
    case NAT_VALUE_STRING:
        self->encoding = find_encoding_by_name(env, encoding->str)->encoding_num;
        break;
    default:
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into %s", NAT_OBJ_CLASS(encoding)->class_name, "String");
    }
    return self;
}

NatObject *String_ref(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *index_obj = args[0];
    NAT_ASSERT_TYPE(index_obj, NAT_VALUE_INTEGER, "Integer");
    int64_t index = NAT_INT_VALUE(index_obj);

    // not sure how we'd handle that given a 64-bit signed int for an index,
    // not to mention that a string that long would be insane to index into
    assert(self->str_len < INT64_MAX);

    NatObject *chars = string_chars(env, self);
    if (index < 0) {
        index = vector_size(&chars->ary) + index;
    }

    if (index < 0 || index >= (int64_t)vector_size(&chars->ary)) {
        return NAT_NIL;
    }
    return static_cast<NatObject *>(vector_get(&chars->ary, index));
}

// FIXME: this is dirty quick solution, a slow algorithm and doesn't account for string encoding
static ssize_t str_index(NatObject *str, NatObject *find, ssize_t start) {
    for (ssize_t i = start; i < str->str_len; i++) {
        if (strncmp(&str->str[i], find->str, find->str_len) == 0) {
            return i;
        }
    }
    return -1;
}

NatObject *String_index(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *find = args[0];
    NAT_ASSERT_TYPE(find, NAT_VALUE_STRING, "String");
    ssize_t index = str_index(self, find, 0);
    if (index == -1) {
        return NAT_NIL;
    }
    return integer(env, index);
}

NatObject *String_sub(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *sub = args[0];
    NatObject *repl = args[1];
    NAT_ASSERT_TYPE(repl, NAT_VALUE_STRING, "String");
    if (NAT_TYPE(sub) == NAT_VALUE_STRING) {
        NatObject *index_obj = String_index(env, self, 1, &sub, NULL);
        if (index_obj == NAT_NIL) {
            return dup(env, self);
        }
        int64_t index = NAT_INT_VALUE(index_obj);
        NatObject *out = string(env, self->str);
        out->str[index] = 0;
        out->str_len = index;
        string_append_string(env, out, repl);
        string_append(env, out, &self->str[index + sub->str_len]);
        return out;
    } else if (NAT_TYPE(sub) == NAT_VALUE_REGEXP) {
        NatObject *match = Regexp_match(env, sub, 1, &self, NULL);
        if (match == NAT_NIL) {
            return dup(env, self);
        }
        NatObject *match_str = MatchData_to_s(env, match, 0, NULL, NULL);
        int64_t index = match->matchdata_region->beg[0];
        NatObject *out = string(env, self->str);
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

NatObject *String_to_i(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    int base = 10;
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], NAT_VALUE_INTEGER, "Integer");
        base = NAT_INT_VALUE(args[0]);
    }
    long long number = str_to_i(self->str, base);
    return integer(env, number);
}

NatObject *String_split(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *splitter = args[0];
    NatObject *ary = array_new(env);
    if (self->str_len == 0) {
        return ary;
    } else if (NAT_TYPE(splitter) == NAT_VALUE_REGEXP) {
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
    } else if (NAT_TYPE(splitter) == NAT_VALUE_STRING) {
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

NatObject *String_ljust(Env *env, NatObject *self, ssize_t argc, NatObject **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *length_obj = args[0];
    NAT_ASSERT_TYPE(length_obj, NAT_VALUE_INTEGER, "Integer");
    ssize_t length = NAT_INT_VALUE(length_obj) < 0 ? 0 : NAT_INT_VALUE(length_obj);
    NatObject *padstr;
    if (argc > 1) {
        padstr = args[1];
        NAT_ASSERT_TYPE(padstr, NAT_VALUE_STRING, "String");
    } else {
        padstr = string(env, " ");
    }
    NatObject *copy = dup(env, self);
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
