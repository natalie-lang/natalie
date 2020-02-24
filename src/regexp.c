#include "natalie.h"
#include "builtin.h"

NatObject *Regexp_new(NatEnv *env, NatObject *self, size_t argc, NatObject **args, struct hashmap *kwargs, NatBlock *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(args[0]) == NAT_VALUE_STRING);
    return nat_regexp(env, args[0]->str);
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
    assert(NAT_TYPE(args[0]) == NAT_VALUE_STRING);
    unsigned char *str = (unsigned char*)args[0]->str;
    int result;
    OnigRegion *region = onig_region_new();
    unsigned char *end = str + strlen((char*)str);
    unsigned char *start = str;
    unsigned char *range = end;
    result = onig_search(self->regexp, str, end, start, range, region, ONIG_OPTION_NONE);
    if (result >= 0) {
        return nat_integer(env, result);
    } else if (result == ONIG_MISMATCH) {
        return NAT_NIL;
    } else {
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        NAT_RAISE(env, nat_const_get(env, NAT_OBJECT, "RuntimeError"), (char*)s);
    }
}
