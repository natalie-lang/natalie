#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Regexp_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (NAT_TYPE(args[0]) == ValueType::Regexp) {
        return regexp(env, args[0]->regexp_str);
    } else {
        NAT_ASSERT_TYPE(args[0], ValueType::String, "String");
        return regexp(env, args[0]->str);
    }
}

Value *Regexp_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    assert(NAT_TYPE(self) == ValueType::Regexp);
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (NAT_TYPE(arg) == ValueType::Regexp && strcmp(self->regexp_str, arg->regexp_str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Regexp_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    assert(NAT_TYPE(self) == ValueType::Regexp);
    Value *out = string(env, "/");
    string_append(env, out, self->regexp_str);
    string_append_char(env, out, '/');
    return out;
}

Value *Regexp_eqtilde(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::Regexp);
    NAT_ASSERT_TYPE(args[0], ValueType::String, "String");
    Value *matchdata = Regexp_match(env, self, argc, args, block);
    if (NAT_TYPE(matchdata) == ValueType::Nil) {
        return matchdata;
    } else {
        assert(matchdata->matchdata_region->num_regs > 0);
        return integer(env, matchdata->matchdata_region->beg[0]);
    }
}

Value *Regexp_match(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    assert(NAT_TYPE(self) == ValueType::Regexp);
    NAT_ASSERT_TYPE(args[0], ValueType::String, "String");
    Value *str_obj = args[0];
    unsigned char *str = (unsigned char *)str_obj->str;
    int result;
    OnigRegion *region = onig_region_new();
    unsigned char *end = str + strlen((char *)str);
    unsigned char *start = str;
    unsigned char *range = end;
    result = onig_search(self->regexp, str, end, start, range, region, ONIG_OPTION_NONE);
    if (result >= 0) {
        env->caller->match = matchdata(env, region, str_obj);
        return env->caller->match;
    } else if (result == ONIG_MISMATCH) {
        env->caller->match = NULL;
        onig_region_free(region, true);
        return NAT_NIL;
    } else {
        env->caller->match = NULL;
        onig_region_free(region, true);
        OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, result);
        NAT_RAISE(env, "RuntimeError", (char *)s);
    }
}

}
