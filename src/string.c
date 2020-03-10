#include "natalie.h"
#include "builtin.h"

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
    nat_string_append(self, str);
    return self;
}

NatObject *String_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(0);
    NatObject *out = nat_string(env, "\"");
    for (size_t i=0; i<self->str_len; i++) {
        // FIXME: iterate over multibyte chars
        char c = self->str[i];
        if (c == '"' || c == '\\' || c == '#') {
            nat_string_append_char(out, '\\');
            nat_string_append_char(out, c);
        } else if (c == '\n') {
            nat_string_append(out, "\\n");
        } else if (c == '\t') {
            nat_string_append(out, "\\t");
        } else {
            nat_string_append_char(out, c);
        }
    }
    nat_string_append_char(out, '"');
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
    nat_string_append(new_str, str);
    return new_str;
}

NatObject *String_mul(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_STRING);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    NAT_ASSERT_TYPE(arg, NAT_VALUE_INTEGER, "Integer");
    NatObject *new_str = nat_string(env, "");
    for (long long i=0; i<NAT_INT_VALUE(arg); i++) {
        nat_string_append_nat_string(new_str, self);
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
    NatObject* arg = args[0];
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

static void nat_succ_string(NatObject *string, char append_char, char begin_char, char end_char) {
    ssize_t index = string->str_len - 1;
    char last_char = string->str[index];
    while (last_char == end_char) {
        string->str[index] = begin_char;
        last_char = string->str[--index];
    }
    if (index == -1) {
        nat_string_append_char(string, append_char);
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
        nat_succ_string(result, 'a', 'a', 'z');
    } else if (last_char == 'Z') {
        nat_succ_string(result, 'A', 'A', 'Z');
    } else if (last_char == '9') {
        nat_succ_string(result, '0', '1', '9');
    } else {
        result->str[index]++;
    }
    return result;
}
