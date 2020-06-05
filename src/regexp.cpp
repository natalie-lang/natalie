#include "natalie/builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *Regexp_new(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (args[0]->is_regexp()) {
        return regexp_new(env, args[0]->as_regexp()->regexp_str);
    } else {
        NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
        return regexp_new(env, args[0]->as_string()->str);
    }
}

Value *Regexp_eqeq(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (arg->is_regexp() && strcmp(self->as_regexp()->regexp_str, arg->as_regexp()->regexp_str) == 0) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Regexp_inspect(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(0);
    StringValue *out = string(env, "/");
    string_append(env, out, self->regexp_str);
    string_append_char(env, out, '/');
    return out;
}

Value *Regexp_eqtilde(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
    Value *match = Regexp_match(env, self, argc, args, block);
    if (match->is_nil()) {
        return match;
    } else {
        MatchDataValue *matchdata = match->as_match_data();
        assert(matchdata->matchdata_region->num_regs > 0);
        return integer(env, matchdata->matchdata_region->beg[0]);
    }
}

Value *Regexp_match(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    RegexpValue *self = self_value->as_regexp();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
    StringValue *str_obj = args[0]->as_string();
    unsigned char *str = (unsigned char *)str_obj->str;
    int result;
    OnigRegion *region = onig_region_new();
    unsigned char *end = str + strlen((char *)str);
    unsigned char *start = str;
    unsigned char *range = end;
    result = onig_search(self->regexp, str, end, start, range, region, ONIG_OPTION_NONE);
    if (result >= 0) {
        env->caller->match = matchdata_new(env, region, str_obj);
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
