#include "natalie.h"
#include "builtin.h"

NatObject *Regexp_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    if (NAT_TYPE(args[0]) == NAT_VALUE_REGEXP) {
        return nat_regexp(env, args[0]->regexp_str);
    } else {
        NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
        return nat_regexp(env, args[0]->str);
    }
}

NatObject *Regexp_eqeq(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NAT_ASSERT_ARGC(1);
    NatObject *arg = args[0];
    if (NAT_TYPE(arg) == NAT_VALUE_REGEXP && strcmp(self->regexp_str, arg->regexp_str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

NatObject *Regexp_inspect(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NatObject *out = nat_string(env, "/");
    nat_string_append(out, self->regexp_str);
    nat_string_append_char(out, '/');
    return out;
}

NatObject *Regexp_eqtilde(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
    NatObject *matchdata = Regexp_match(env, self, argc, args, kwargs, block);
    if (NAT_TYPE(matchdata) == NAT_VALUE_NIL) {
        return matchdata;
    } else {
        assert(matchdata->matchdata_region->num_regs > 0);
        return nat_integer(env, matchdata->matchdata_region->beg[0]);
    }
}

NatObject *Regexp_match(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == NAT_VALUE_REGEXP);
    NAT_ASSERT_TYPE(args[0], NAT_VALUE_STRING, "String");
    NatObject *str_obj = args[0];
    unsigned char *str = (unsigned char*)str_obj->str;
    int result;
    OnigRegion *region = onig_region_new();
    unsigned char *end = str + strlen((char*)str);
    unsigned char *start = str;
    unsigned char *range = end;
    result = onig_search(self->regexp, str, end, start, range, region, ONIG_OPTION_NONE);
    if (result >= 0) {
        return nat_matchdata(env, region, str_obj);
    } else if (result == ONIG_MISMATCH) {
        return NAT_NIL;
    } else {
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "RuntimeError"), (char*)s);
    }
}
