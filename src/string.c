#include "builtin.h"
#include "natalie.h"
#include <ctype.h>

NatObject *String_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC_AT_MOST(1);
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
        return nat_string(env, args[0]->str);
    } else {
        return nat_string(env, "");
    }
}

NatObject *String_to_s(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(0);
    return self;
}

NatObject *String_ltlt(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_NOT_FROZEN(self);
    NatObject *arg = args[0];
    char *str;
    if (NAT_TYPE(arg) == NAT_VALUE_STRING) {
        str = arg->str;
    } else {
        NatObject *str_obj = nat_send(env, arg, "to_s", 0, NULL, NULL);
        NAT_ASSERT_TYPE(str_obj, NAT_VALUE_STRING, "String");
        str = str_obj->str;
    }
    nat_string_append(env, self, str);
    return self;
}

NatObject *String_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(0);
    NatObject *out = nat_string(env, "\"");
    for (size_t i = 0; i < self->str_len; i++) {
        char c = self->str[i];
        if (c == '"' || c == '\\' || c == '#') {
            nat_string_append_char(env, out, '\\');
            nat_string_append_char(env, out, c);
        } else if (c == '\n') {
            nat_string_append(env, out, "\\n");
        } else if (c == '\t') {
            nat_string_append(env, out, "\\t");
        } else {
            nat_string_append_char(env, out, c);
        }
    }
    nat_string_append_char(env, out, '"');
    return out;
}

NatObject *String_add(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    char *str;
    if (NAT_TYPE(arg) == NAT_VALUE_STRING) {
        str = arg->str;
    } else {
        NatObject *str_obj = nat_send(env, arg, "to_s", 0, NULL, NULL);
        NAT_ASSERT_TYPE(str_obj, NAT_VALUE_STRING, "String");
        str = str_obj->str;
    }
    NatObject *new_str = nat_string(env, self->str);
    nat_string_append(env, new_str, str);
    return new_str;
}

NatObject *String_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    NatObject *new_str = nat_string(env, "");
    for (long long i = 0; i < NAT_INT_VALUE(arg); i++) {
        nat_string_append_nat_string(env, new_str, self);
    }
    return new_str;
}

NatObject *String_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_STRING && strcmp(self->str, arg->str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *String_cmp(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
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
    return nat_integer(env, result);
}

NatObject *String_eqtilde(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_REGEXP, "Regexp");
    return Regexp_eqtilde(env, args[0], 1, &self, NULL, block);
}

NatObject *String_match(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_REGEXP, "Regexp");
    return Regexp_match(env, args[0], 1, &self, NULL, block);
}

static void nat_succ_string(NatEnv *env, NatObject *string, char append_char, char begin_char, char end_char) {
    ssize_t index = string->str_len - 1;
    char last_char = string->str[index];
    while (last_char == end_char) {
        string->str[index] = begin_char;
        last_char = string->str[--index];
    }
    if (index == -1) {
        nat_string_append_char(env, string, append_char);
    } else {
        string->str[index]++;
    }
}

NatObject *String_succ(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *result = nat_string(env, self->str);
    ssize_t index = self->str_len - 1;
    char last_char = self->str[index];
    if (last_char == 'z') {
        nat_succ_string(env, result, 'a', 'a', 'z');
    } else if (last_char == 'Z') {
        nat_succ_string(env, result, 'A', 'A', 'Z');
    } else if (last_char == '9') {
        nat_succ_string(env, result, '0', '1', '9');
    } else {
        result->str[index]++;
    }
    return result;
}

NatObject *String_ord(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NatObject *chars = nat_string_chars(env, self);
    if (chars->ary_len == 0) {
        NAT_RAISE(env, "ArgumentError", "empty string");
    }
    NatObject *c = chars->ary[0];
    assert(NAT_TYPE(c) == NAT_VALUE_STRING);
    assert(c->str_len > 0);
    unsigned int code;
    switch (c->str_len) {
    case 0:
        NAT_UNREACHABLE();
    case 1:
        code = (unsigned char)c->str[0];
        return nat_integer(env, code);
    case 2:
        code = (((unsigned char)c->str[0] ^ 0xC0) << 6) + (((unsigned char)c->str[1] ^ 0x80) << 0);
        return nat_integer(env, code);
    case 3:
        code = (((unsigned char)c->str[0] ^ 0xE0) << 12) + (((unsigned char)c->str[1] ^ 0x80) << 6) + (((unsigned char)c->str[2] ^ 0x80) << 0);
        return nat_integer(env, code);
    case 4:
        code = (((unsigned char)c->str[0] ^ 0xF0) << 18) + (((unsigned char)c->str[1] ^ 0x80) << 12) + (((unsigned char)c->str[2] ^ 0x80) << 6) + (((unsigned char)c->str[3] ^ 0x80) << 0);
        return nat_integer(env, code);
    default:
        NAT_UNREACHABLE();
    }
}

NatObject *String_bytes(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *ary = nat_array(env);
    for (size_t i = 0; i < self->str_len; i++) {
        nat_array_push(env, ary, nat_integer(env, self->str[i]));
    }
    return ary;
}

NatObject *String_chars(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    return nat_string_chars(env, self);
}

NatObject *String_encoding(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *Encoding = nat_const_get(env, NAT_OBJECT, "Encoding", true);
    switch (self->encoding) {
    case NAT_ENCODING_ASCII_8BIT:
        return nat_const_get(env, Encoding, "ASCII_8BIT", true);
    case NAT_ENCODING_UTF_8:
        return nat_const_get(env, Encoding, "UTF_8", true);
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

static NatObject *find_encoding_by_name(NatEnv *env, char *name) {
    char *lcase_name = lcase_string(name);
    NatObject *list = Encoding_list(env, nat_const_get(env, NAT_OBJECT, "Encoding", true), 0, NULL, NULL, NULL);
    for (size_t i = 0; i < list->ary_len; i++) {
        NatObject *encoding = list->ary[i];
        NatObject *names = encoding->encoding_names;
        for (size_t n = 0; n < names->ary_len; n++) {
            char *name = lcase_string(names->ary[n]->str);
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

NatObject *String_force_encoding(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
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

NatObject *String_ref(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NatObject *index_obj = args[0];
    NAT_ASSERT_TYPE(index_obj, NAT_VALUE_INTEGER, "Integer");
    int64_t index = NAT_INT_VALUE(index_obj);

    // not sure how we'd handle that given a 64-bit signed int for an index,
    // not to mention that a string that long would be insane to index into
    assert(self->str_len < INT64_MAX);

    NatObject *chars = nat_string_chars(env, self);
    if (index < 0) {
        index = chars->ary_len + index;
    }

    if (index < 0 || index >= (int64_t)chars->ary_len) {
        return NAT_NIL;
    }
    return chars->ary[index];
}
